/*
 * Copyright (c) 2012 Aldebaran Robotics. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the COPYING file.
 */

#include <qi/log/fileloghandler.hpp>

#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>

#include <sstream>
#include <string>
#include <qi/log.hpp>
#include <qi/os.hpp>
#include <cstdio>

#define CATSIZEMAX 16

namespace qi {
  namespace log {
    class PrivateFileLogHandler
    {
    public:
      void cutCat(const char* category, char* res);

      FILE* _file;
    };


    FileLogHandler::FileLogHandler(const std::string& filePath)
      : _private(new PrivateFileLogHandler)
    {
      _private->_file = NULL;
      boost::filesystem::path fPath(filePath);
      // Create the directory!
      try
      {
        if (!boost::filesystem::exists(fPath.make_preferred().parent_path()))
          boost::filesystem::create_directories(fPath.make_preferred().parent_path());
      }
      catch (const boost::filesystem::filesystem_error &e)
      {
        qiLogWarning("qi.log.fileloghandler") << e.what() << std::endl;
      }

      // Open the file.
      FILE* file = qi::os::fopen(fPath.make_preferred().string().c_str(), "w+");

      if (file)
        _private->_file = file;
      else
        qiLogWarning("qi.log.fileloghandler") << "Cannot open "
                                              << filePath << std::endl;
    }

    FileLogHandler::~FileLogHandler()
    {
      if (_private->_file != NULL)
        fclose(_private->_file);
    }

    void PrivateFileLogHandler::cutCat(const char* category, char* res)
    {
      int categorySize = strlen(category);
      if (categorySize < CATSIZEMAX)
      {
        memset(res, ' ', CATSIZEMAX);
        memcpy(res, category, strlen(category));
      }
      else
      {
        memset(res, '.', CATSIZEMAX);
        memcpy(res + 3, category + categorySize - CATSIZEMAX + 3, CATSIZEMAX - 3);
      }
      res[CATSIZEMAX] = '\0';
    }


    void FileLogHandler::log(const qi::log::LogLevel verb,
                             const qi::os::timeval   date,
                             const char              *category,
                             const char              *msg,
                             const char              *file,
                             const char              *fct,
                             const int               line)
    {
      if (verb > qi::log::verbosity() || _private->_file == NULL)
      {
        return;
      }
      else
      {
        const char* head = logLevelToString(verb);
        char fixedCategory[CATSIZEMAX + 1];
        fixedCategory[CATSIZEMAX] = '\0';
        _private->cutCat(category, fixedCategory);

        std::stringstream ss;
        ss << date.tv_sec << "." << date.tv_usec;

        fprintf(_private->_file,"%s ", head);
        int ctx = qi::log::context();
        switch (ctx)
        {
        case 1:
          fprintf(_private->_file, "%s: ", fixedCategory);
          break;
        case 2:
          fprintf(_private->_file, "%s ", ss.str().c_str());
          break;
        case 3:
          fprintf(_private->_file, "%s(%d) ", file, line);
          break;
        case 4:
          fprintf(_private->_file, "%s %s: ", ss.str().c_str(), fixedCategory);
          break;
        case 5:
          fprintf(_private->_file, "%s %s(%d) ", ss.str().c_str(), file, line);
          break;
        case 6:
          fprintf(_private->_file, "%s: %s(%d) ", fixedCategory, file, line);
          break;
        case 7:
          fprintf(_private->_file, "%s %s: %s(%d) %s ", ss.str().c_str(), fixedCategory, file, line, fct);
          break;
        default:
          break;
        }
        fprintf(_private->_file,"%s", msg);

        fflush(_private->_file);
      }
    }
  }
}
