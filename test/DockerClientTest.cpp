#include <cstdlib>
#include <fstream>

#include "DockerClient.hpp"
#include "gtest/gtest.h"

using namespace DockerClientpp;

class LaunchTest : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    std::system("docker rm -f test > /dev/null 2>&1");
  }
  virtual void TearDown() override {
    std::system("docker start test > /dev/null 2>&1");
  }
  void test(DockerClient &dc) {
    string id;
    id = dc.createContainer({{"AttachStdout", true},
                             {"AttachStderr", false},
                             {"Tty", true},
                             {"StopSignal", "SIGKILL"},
                             {"Image", "busybox:1.26"}},
                            "test");
    ASSERT_FALSE(id.empty()) << id;

    dc.startContainer("test");
    ASSERT_TRUE(std::system(("docker ps -q --no-trunc | grep " + id +
                             " > /dev/null 2>&1")
                                .c_str()) == 0);

    dc.stopContainer("test");
    ASSERT_FALSE(std::system(("docker ps -q --no-trunc | grep " + id +
                              " > /dev/null 2>&1")
                                 .c_str()) == 0);
  }
};

TEST_F(LaunchTest, UnixSocketTest) {
  DockerClient dc(DockerClientpp::UNIX);
  test(dc);
}

TEST_F(LaunchTest, TCPSocketTest) {
  DockerClient dc(DockerClientpp::TCP, "127.0.0.1:8888");
  test(dc);
}

TEST(ExecTest, CreateExecTest) {
  DockerClient dc;  //(TCP, "127.0.0.1:8888");
  string id;
  for (int i = 0; i < 5; i++) {
    id = dc.createExecution("test", {{"AttachStdout", true},
                                     {"AttachStderr", true},
                                     {"Tty", false},
                                     {"Cmd", json::array({"echo", "1"})}});
    ASSERT_FALSE(id.empty()) << id;
    auto output = dc.startExecution(id, {{"Detach", false}, {"Tty", false}});
    EXPECT_EQ("1\n", output);
  }
  // std::system("docker ps -a");
}

TEST(ExecTest, InspectTest) {
  DockerClient dc;
  string id;
  id = dc.createExecution("test", {{"AttachStdout", true},
                                   {"AttachStderr", true},
                                   {"Tty", false},
                                   {"Cmd", json::array({"echo", "1"})}});
  ASSERT_FALSE(id.empty()) << id;
  dc.startExecution(id, {{"Detach", false}, {"Tty", false}});
  json status = json::parse(dc.inspectExecution(id));
  EXPECT_EQ(0, status["ExitCode"].get<int>());
}

TEST(ExecTest, PutFileTest) {
  DockerClient dc;  //(TCP, "127.0.0.1:8888");
  string id;
  std::fstream fs("1", std::fstream::out);
  fs << 1 << std::endl;
  fs.close();
  fs.open("2", std::fstream::out);
  fs << 2 << std::endl;
  fs.close();
  fs.open("3", std::fstream::out);
  fs << 3 << std::endl;
  fs.close();

  dc.putFiles("test", {"1", "2"}, "/tmp");

  id = dc.createExecution("test", {{"AttachStdout", true},
                                   {"AttachStderr", true},
                                   {"Tty", false},
                                   {"Cmd", json::array({"ls", "/tmp"})}});
  ASSERT_FALSE(id.empty()) << id;
  auto output = dc.startExecution(id, {{"Detach", false}, {"Tty", false}});
  EXPECT_EQ("1\n2\n", output);

  dc.putFiles("test", {"1", "2", "3"}, "/tmp");

  id = dc.createExecution("test", {{"AttachStdout", true},
                                   {"AttachStderr", true},
                                   {"Tty", false},
                                   {"Cmd", json::array({"ls", "/tmp"})}});
  ASSERT_FALSE(id.empty()) << id;
  output = dc.startExecution(id, {{"Detach", false}, {"Tty", false}});
  EXPECT_EQ("1\n2\n3\n", output);

  std::remove("1");
  std::remove("2");
  std::remove("3");
}

TEST(ExecTest, GetFileTest) {
  std::system("docker exec test sh -c 'echo 123 > 1'");
  DockerClient dc;
  dc.getFile("test", "/1", "./");
  std::system("docker exec test rm 1");

  std::fstream fs("1", std::fstream::in);
  string content;
  fs >> content;
  EXPECT_EQ("123", content);
}
