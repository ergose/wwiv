#include <cstdio>
#include <string>
#include "core/command_line.h"
#include "core/file.h"
#include "core/log.h"
#include "core/os.h"
#include "gtest/gtest.h"

using std::string;
using namespace wwiv::core;

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  wwiv::core::Logger::Init(argc, argv);
  CommandLine cmdline(argc, argv, "net");
  cmdline.AddStandardArgs();
  cmdline.add_argument({ "wwiv_test_tempdir", "Use instead of WWIV_TEST_TEMPDIR environment variable.", "" });
  cmdline.set_no_args_allowed(true);
  if (!cmdline.Parse()) {
    LOG(ERROR) << "Failed to parse cmdline.";
  }

  tzset();

  string tmpdir = cmdline.arg("wwiv_test_tempdir").as_string();
  if (tmpdir.empty()) {
    tmpdir = wwiv::os::environment_variable("WWIV_TEST_TEMPDIR");
  }
  if (tmpdir.empty()) {
    FAIL() << "WWIV_TEST_TEMPDIR must be set for this test suite.";
  } 
  if (!File::Exists(tmpdir)) {
    File::mkdirs(tmpdir);
  }

  return RUN_ALL_TESTS();
}
