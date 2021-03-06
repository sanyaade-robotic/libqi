/*
 * Copyright (c) 2012 Aldebaran Robotics. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the COPYING file.
 */


#include <boost/filesystem.hpp>
#include <locale>
#include <cstdio>
#include <cstdlib>
#include <unistd.h> //gethostname
#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <limits.h>

#ifdef _WIN32
# include <io.h>      //_wopen
# include <windows.h> //Sleep
#else
# include <pwd.h>
# include <sys/time.h>
#endif

#include <qi/os.hpp>
#include <qi/error.hpp>
#include <qi/qi.hpp>
#include "src/filesystem.hpp"
#include "utils.hpp"

namespace qi {
  namespace os {

    FILE* fopen(const char *filename, const char *mode) {
      return ::fopen(boost::filesystem::path(filename, qi::unicodeFacet()).string(qi::unicodeFacet()).c_str(),
                     boost::filesystem::path(mode, qi::unicodeFacet()).string(qi::unicodeFacet()).c_str());

    }

    int stat(const char *filename, struct ::stat* status) {
      return ::stat(boost::filesystem::path(filename, qi::unicodeFacet()).string(qi::unicodeFacet()).c_str(), status);
    }

    std::string getenv(const char *var) {
      char *res = ::getenv(boost::filesystem::path(var, qi::unicodeFacet()).string(qi::unicodeFacet()).c_str());
      if (res == NULL)
        return "";
      return std::string(res);
    }

    int setenv(const char *var, const char *value) {
      return ::setenv(boost::filesystem::path(var, qi::unicodeFacet()).string(qi::unicodeFacet()).c_str(),
                      boost::filesystem::path(value, qi::unicodeFacet()).string(qi::unicodeFacet()).c_str(), 1);
    }

    void sleep(unsigned int seconds) {
      // In case sleep was interrupted by a signal,
      // keep sleeping until we have slept the correct amount
      // of time.
      while (seconds != 0) {
        seconds = ::sleep(seconds);
      }
    }

    void msleep(unsigned int milliseconds) {
      // Not the same for usleep: it returns a non-zero
      // error code if it was interupted...
      usleep(milliseconds * 1000);
    }


    std::string home()
    {
      std::string envHome = qi::os::getenv("HOME");
      if (envHome != "")
      {
        return boost::filesystem::path(envHome, qi::unicodeFacet()).make_preferred().string(qi::unicodeFacet());
      }

      // $HOME environment variable not defined:
      char *lgn;
      struct passwd *pw;
      if ((lgn = getlogin()) == NULL || (pw = getpwnam(lgn)) == NULL)
      {
        return boost::filesystem::path(pw->pw_dir, qi::unicodeFacet()).make_preferred().string(qi::unicodeFacet());
      }

      // Give up:
      return "";
    }

    std::string mktmpdir(const char *prefix)
    {
      std::string sprefix;
      std::string tmpdir;
      std::string path;
      int         i = 0;

      if (prefix)
        sprefix = prefix;

      do
      {
        tmpdir = sprefix;
        tmpdir += randomstr(7);
        boost::filesystem::path pp(qi::os::tmp(), qi::unicodeFacet());
        pp.append(tmpdir, qi::unicodeFacet());
        path = pp.make_preferred().string(qi::unicodeFacet());
        ++i;
      }
      while (mkdir(path.c_str(), S_IRWXU) == -1 && i < TMP_MAX);

      return path;
    }

    std::string tmp()
    {
      std::string temp = ::qi::os::getenv("TMPDIR");
      if (temp.empty())
        temp = "/tmp/";

      boost::filesystem::path p = boost::filesystem::path(temp, qi::unicodeFacet());

      return p.string(qi::unicodeFacet());
    }

    int gettimeofday(qi::os::timeval *tp) {
      struct ::timeval tv;
      int ret = ::gettimeofday(&tv, 0);
      tp->tv_sec = tv.tv_sec;
      tp->tv_usec = tv.tv_usec;
      return ret;
    }

    std::string tmpdir(const char *prefix)
    {
      return mktmpdir(prefix);
    }

    std::string gethostname()
    {
      long hostNameMax;
#ifdef HAVE_SC_HOST_NAME_MAX
      hostNameMax = sysconf(_SC_HOST_NAME_MAX) + 1;
#else
      hostNameMax = HOST_NAME_MAX + 1;
#endif
      char *szHostName = (char*) malloc(hostNameMax * sizeof(char));
      memset(szHostName, 0, hostNameMax);
      if (::gethostname(szHostName, hostNameMax) == 0) {
        std::string hostname(szHostName);
        free(szHostName);
        return hostname;
      }

      free(szHostName);
      return std::string();
    }
  };
};
