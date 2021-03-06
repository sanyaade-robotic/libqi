#include <gtest/gtest.h>
#include <qi/os.hpp>
#include <qi/path.hpp>
#include <qi/qi.hpp>


#ifdef _WIN32
# include <windows.h>
# include <process.h>
#else
# include <sys/wait.h>
#endif

/*
 * Copyright (c) 2012 Aldebaran Robotics. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the COPYING file.
 */
// TODO2: Some tests to check the consume stack

static std::string binDir;

TEST(spawnvp, CmdWithNoArgs)
{
  char* args[] = {(char*)binDir.c_str(), NULL};

  int status = 0;

  int childPid = qi::os::spawnvp(args);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 42) << "status: " << status;
}

TEST(spawnvp, CmdWithArgs)
{
  char* args[] = {(char*)binDir.c_str(), (char*)"23", NULL};

  int status = 0;

  int childPid = qi::os::spawnvp(args);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 23) << "status: " << status;
}

TEST(spawnvp, CmdWithMultiArgs)
{
  char* args[] = {(char*)binDir.c_str(), (char*)"23", (char*)"-1", NULL};

  int status = 0;

  int childPid = qi::os::spawnvp(args);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 22) << "status: " << status;
}

TEST(spawnvp, InvalidBin)
{
  char* args[] = {(char*)"test42", NULL};

  int status = 0;

  int childPid = qi::os::spawnvp(args);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 127) << "status: " << status;
}


TEST(spawnlp, CmdWithNoArgs)
{
  std::string bin = binDir.c_str();

  int status = 0;

  int childPid = qi::os::spawnlp(bin.c_str(), NULL);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 42) << "status: " << status;
}

TEST(spawnlp, CmdWithArgs)
{
  std::string bin = binDir.c_str();
  std::string arg1 = "23";
  int status = 0;

  int childPid = qi::os::spawnlp(bin.c_str(), arg1.c_str(), NULL);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 23) << "status: " << status;
}

TEST(spawnlp, CmdWithMultiArgs)
{
  std::string bin = binDir.c_str();
  std::string arg1 = "23";
  std::string arg2 = "-1";

  int status = 0;

  int childPid = qi::os::spawnlp(bin.c_str(), arg2.c_str(), arg1.c_str(), NULL);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 22) << "status: " << status;
}

TEST(spawnlp, Environment)
{
  // The check_env executable exits with rectode 1 if
  // the FOO environment var is not equal to BAR.
  std::string checkEnv = qi::path::findBin("check_env");
  int status = 0;
  qi::os::setenv("FOO", "BAR");
  int childPid = qi::os::spawnlp(checkEnv.c_str(), NULL);
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_EQ(0, status) << "waitpid failed with retcode:   " << error;
  EXPECT_EQ(0, status) << "check_env failed with rectode: " << status;
}



TEST(spawnlp, InvalidBin)
{
  std::string bin = "test42";
  int status = 0;

  int childPid = qi::os::spawnlp(bin.c_str());
  int error = qi::os::waitpid(childPid, &status);

  EXPECT_TRUE(error == 0) << "error: " << error;
  EXPECT_TRUE(status == 127) << "status: " << status;
}

TEST(system, CmdWithNoArgs)
{
  std::string bin = binDir;

  int status = qi::os::system(bin.c_str());

  EXPECT_TRUE(status == 42) << "status: " << status;
}

TEST(system, CmdWithArgs)
{
  std::string bin = binDir + " 23";

  int status = qi::os::system(bin.c_str());

  EXPECT_TRUE(status == 23) << "status: " << status;
}

TEST(system, CmdWithMultiArgs)
{
  std::string bin = binDir + " 23 -1";

  int status = qi::os::system(bin.c_str());

  EXPECT_TRUE(status == 22) << "status: " << status;
}

TEST(system, InvalidBin)
{
  std::string bin = "test42";

  int status = qi::os::system(bin.c_str());

 #ifdef _WIN32
  EXPECT_TRUE(status == 1) << "status: " << status;
 #else
  EXPECT_TRUE(status == 127) << "status: " << status;
 #endif
}


int main(int argc, char* argv[])
{
  qi::init(argc, (char**)argv);
  binDir = qi::path::findBin("testlaunch");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
