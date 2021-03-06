/*
 * Copyright (c) 2012 Aldebaran Robotics. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the COPYING file.
 */

#include <qi/log.hpp>
#include <qi/os.hpp>
#include <list>
#include <map>
#include <cstring>

#include <qi/log/consoleloghandler.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/lockfree/fifo.hpp>
#include <boost/function.hpp>

#define RTLOG_BUFFERS (128)

#define CAT_SIZE 64
#define FILE_SIZE 128
#define FUNC_SIZE 64
#define LOG_SIZE 2048

namespace qi {
  namespace log {

    typedef struct sPrivateLog
    {
      LogLevel        _logLevel;
      char            _category[CAT_SIZE];
      char            _file[FILE_SIZE];
      char            _function[FUNC_SIZE];
      int             _line;
      char            _log[LOG_SIZE];
      qi::os::timeval _date;
    } privateLog;

    class Log
    {
    public:
      inline Log();
      inline ~Log();

      void run();
      void printLog();

    public:
      bool                       LogInit;
      boost::thread              LogThread;
      boost::mutex               LogWriteLock;
      boost::mutex               LogHandlerLock;
      boost::condition_variable  LogReadyCond;

      boost::lockfree::fifo<privateLog*>     logs;
      std::map<std::string, logFuncHandler > logHandlers;
    };

    static LogLevel               _glVerbosity = qi::log::info;
    static int                    _glContext = false;
    static bool                   _glSyncLog = false;
    static bool                   _glInit    = false;
    static bool                   _glAtExit  = false;
    static ConsoleLogHandler      *_glConsoleLogHandler;

    static Log                    *LogInstance;
    static privateLog             LogBuffer[RTLOG_BUFFERS];
    static volatile unsigned long LogPush = 0;

    static class DefaultLogInit
    {
    public:
      DefaultLogInit()
      {
        _glInit = false;
        qi::log::init();
      }

      ~DefaultLogInit()
      {
        qi::log::destroy();
      };
    } synchLog;

    void Log::printLog()
    {
      privateLog* pl;
      boost::mutex::scoped_lock lock(LogHandlerLock);
      while (logs.dequeue(&pl))
      {
        if (!logHandlers.empty())
        {
          std::map<std::string, logFuncHandler >::iterator it;
          for (it = logHandlers.begin();
               it != logHandlers.end(); ++it)
          {
            (*it).second(pl->_logLevel,
                         pl->_date,
                         pl->_category,
                         pl->_log,
                         pl->_file,
                         pl->_function,
                         pl->_line);
          }
        }
      }
    }

    void Log::run()
    {
      while (LogInit)
      {
        {
          boost::mutex::scoped_lock lock(LogWriteLock);
          LogReadyCond.wait(lock);
        }

        printLog();
      }
    };

    inline Log::Log()
    {
      LogInit = true;
      if (!_glSyncLog)
        LogThread = boost::thread(&Log::run, this);
    };

    inline Log::~Log()
    {
      if (!LogInit)
        return;

      LogInit = false;

      if (!_glSyncLog)
      {
        LogThread.interrupt();
        LogThread.join();

        printLog();
      }
    }

    static void my_strcpy_log(char *dst, const char *src, int len) {
      if (!src)
        src = "(null)";

      int messSize = strlen(src);
      // check if the last char is a \n
      if (src[messSize - 1] == '\n')
      {
        // Get the size to memcpy (don't forget we need 1 space more for \0)
        int strSize = messSize < len  ? messSize : len - 1;
       #ifndef _WIN32
        memcpy(dst, src, strSize);
       #else
        memcpy_s(dst, len,  src, strSize);
       #endif
        dst[strSize] = 0;
        return;
      }

     #ifndef _WIN32
      // Get the size to memcpy (we need 2 spaces more for \n\0)
      int strSize = messSize < len - 1  ? messSize : len - 2;
      memcpy(dst, src, strSize);
      dst[strSize] = '\n';
      dst[strSize + 1] = '\0';
     #else
      // Get the size to memcpy (we need 3 spaces more for \r\n\0)
      int strSize = messSize < len - 2  ? messSize : len - 3;
      memcpy_s(dst, len,  src, strSize);
      dst[strSize] = '\r';
      dst[strSize + 1] = '\n';
      dst[strSize + 2] = '\0';
     #endif
    }

    static void my_strcpy(char *dst, const char *src, int len) {
      if (!src)
        src = "(null)";
     #ifdef _MSV_VER
      strncpy_s(dst, len, src, _TRUNCATE);
     #else
      strncpy(dst, src, len);
      dst[len - 1] = 0;
     #endif
    }

    void init(qi::log::LogLevel verb,
              int ctx,
              bool synchronous)
    {
      setVerbosity(verb);
      setContext(ctx);
      setSynchronousLog(synchronous);

      if (_glInit)
        destroy();

      _glConsoleLogHandler = new ConsoleLogHandler;
      LogInstance          = new Log;
      addLogHandler("consoleloghandler",
                    boost::bind(&ConsoleLogHandler::log,
                                _glConsoleLogHandler,
                                _1, _2, _3, _4, _5, _6, _7));
      _glInit = true;
    }

    void destroy()
    {
      if (!_glInit)
        return;
      _glInit = false;
      LogInstance->printLog();
      delete _glConsoleLogHandler;
      _glConsoleLogHandler = 0;
      delete LogInstance;
      LogInstance = 0;
    }

    void flush()
    {
      if (_glInit)
        LogInstance->printLog();
    }

    void log(const LogLevel        verb,
             const char           *category,
             const char           *msg,
             const char           *file,
             const char           *fct,
             const int             line)

    {
      if (!LogInstance)
        return;
      if (!LogInstance->LogInit)
        return;

      int tmpRtLogPush = ++LogPush % RTLOG_BUFFERS;
      privateLog* pl = &(LogBuffer[tmpRtLogPush]);

      qi::os::timeval tv;
      qi::os::gettimeofday(&tv);

      pl->_logLevel = verb;
      pl->_line = line;
      pl->_date.tv_sec = tv.tv_sec;
      pl->_date.tv_usec = tv.tv_usec;

      my_strcpy(pl->_category, category, CAT_SIZE);
      my_strcpy(pl->_file, file, FILE_SIZE);
      my_strcpy(pl->_function, fct, FUNC_SIZE);
      my_strcpy_log(pl->_log, msg, LOG_SIZE);

      if (_glSyncLog)
      {
        if (!LogInstance->logHandlers.empty())
        {
          std::map<std::string, logFuncHandler >::iterator it;
          for (it = LogInstance->logHandlers.begin();
               it != LogInstance->logHandlers.end(); ++it)
          {
            (*it).second(pl->_logLevel,
                         pl->_date,
                         pl->_category,
                         pl->_log,
                         pl->_file,
                         pl->_function,
                         pl->_line);
          }
        }
      }
      else
      {
        LogInstance->logs.enqueue(pl);
        LogInstance->LogReadyCond.notify_one();
      }
    }

    void addLogHandler(const std::string& name, logFuncHandler fct)
    {
      if (!LogInstance)
        return;
      boost::mutex::scoped_lock l(LogInstance->LogHandlerLock);
      LogInstance->logHandlers[name] = fct;
    }

    void removeLogHandler(const std::string& name)
    {
      if (!LogInstance)
        return;
      boost::mutex::scoped_lock l(LogInstance->LogHandlerLock);
      LogInstance->logHandlers.erase(name);
    }

    const LogLevel stringToLogLevel(const char* verb)
    {
      std::string v(verb);
      if (v == "silent")
        return qi::log::silent;
      if (v == "fatal")
        return qi::log::fatal;
      if (v == "error")
        return qi::log::error;
      if (v == "warning")
        return qi::log::warning;
      if (v == "info")
        return qi::log::info;
      if (v == "verbose")
        return qi::log::verbose;
      if (v == "debug")
        return qi::log::debug;
      return qi::log::info;
    }

    const char *logLevelToString(const LogLevel verb)
    {
      static const char *sverb[] = {
        "[SILENT]", // never shown
        "[FATAL]",
        "[ERROR]",
        "[WARN ]",
        "[INFO ]",
        "[VERB ]",
        "[DEBUG]"
      };
      return sverb[verb];
    }

    void setVerbosity(const LogLevel lv)
    {
      const char *verbose = std::getenv("VERBOSE");

      if (verbose)
        _glVerbosity = (LogLevel)atoi(verbose);
      else
        _glVerbosity = lv;
    };

    LogLevel verbosity()
    {
      return _glVerbosity;
    };

    void setContext(int ctx)
    {
      const char *context = std::getenv("CONTEXT");

      if (context)
        _glContext = (atoi(context));
      else
        _glContext = ctx;
    };

    int context()
    {
      return _glContext;
    };

    void setSynchronousLog(bool sync)
    {
      _glSyncLog = sync;
    };

  } // namespace log
} // namespace qi

