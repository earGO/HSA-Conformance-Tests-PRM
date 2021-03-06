/*
   Copyright 2014-2015 Heterogeneous System Architecture (HSA) Foundation

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "HexlTestRunner.hpp"
#include "Stats.hpp"
#include "HexlTest.hpp"
#include "HexlResource.hpp"
#include "RuntimeCommon.hpp"
#include <sstream>
#include "Utils.hpp"
#include <time.h>

namespace hexl {

TestRunnerBase::TestRunnerBase(Context* context_)
  : TestRunner(context_), testContext(0)
{
}

void TestRunnerBase::Init()
{
}

void TestRunnerBase::RunTest(const std::string& path, Test* test)
{
  assert(test);
  Init();
  BeforeTest(path, test);
  TestResult result = ExecuteTest(test);
  t_end = clock();
  result.SetTime(t_begin, t_end);
  AfterTest(path, test, result);
}

void TestRunnerBase::RunTestSpec(const std::string& path, TestSpec* spec)
{
  spec->InitContext(context);
  t_begin = clock();
  Test *test = spec->Create();
  RunTest(path, test);
  if (test) { delete test; }
  delete spec;
}

void TestRunnerBase::BeforeTest(const std::string& path, Test* test)
{
  std::string fullTestName = path + "/" + test->TestName();
  test->InitContext(context);
  testContext = test->GetContext();
  testContext->Put("hexl.outputPath", fullTestName);
  testContext->Put("hexl.log.stream.debug", TestOut());
  testContext->Put("hexl.log.stream.info", TestOut());
  testContext->Put("hexl.log.stream.error", TestOut());
  testContext->Info() << "START:  " << fullTestName << std::endl;
  if (testContext->IsVerbose("description")) {
    testContext->Info() << "Test description:" << std::endl;
    IndentStream indent(testContext->Info());
    test->Description(testContext->Info());
  }
  testContext->PrintIfVerbose("context", "Test context", test->GetContext());
  if (testContext->IsDumpEnabled("context")) {
    testContext->Dump();
  }
  if (testContext->IsDumpEnabled("hxl", false)) {
    std::ostream* out = testContext->RM()->GetOutput(testContext->GetOutputName(fullTestName, "hxl"));
    if (out) {
      test->Serialize(*out);
      delete out;
    }
  }
  testContext->Stats().Clear();
}

void TestRunnerBase::AfterTest(const std::string& path, Test* test, const TestResult& result)
{
  result.IncStats(stats);
  std::string fullTestName = path + "/" + test->TestName();
  test->GetContext()->Info() <<
    result.StatusString() << ": " <<
    fullTestName << std::endl;
}

class TestRunnerExecute : public TestSpecIterator {
private:
  TestRunnerBase* runner;

public:
  explicit TestRunnerExecute(TestRunnerBase* runner_)
    : runner(runner_) { }

  void operator()(const std::string& path, TestSpec* spec) override
  {
    spec->InitContext(runner->GetContext());
    if (spec->IsValid()) {
      runner->RunTestSpec(path, spec);
    } else {
      delete spec;
    }
  }
};

bool TestRunnerBase::RunTests(TestSet& tests)
{
  Init();
  if (!BeforeTestSet(tests)) { return false; }
  TestRunnerExecute exec(this);
  tests.Iterate(exec);
  if (!AfterTestSet(tests)) { return false; }
  return true;
}

TestResult TestRunnerBase::ExecuteTest(Test* test)
{
  test->Run();
  return test->Result();
}

bool SimpleTestRunner::AfterTestSet(TestSet& testSet)
{
  std::cout << "Testrun statistics:" << std::endl;
  IndentStream indent(std::cout);
  stats.PrintTestSet(std::cout);
  return true;
}

void SimpleTestRunner::AfterTest(const std::string& path, Test* test, const TestResult& result)
{
  TestRunnerBase::AfterTest(path, test, result);
  std::cout << std::endl;
}

HTestRunner::HTestRunner(Context* context_)
  : TestRunnerBase(context_)
{
  testLogLevel = context->Opts()->GetUnsigned("testloglevel", 4);
}

void HTestRunner::BeforeTest(const std::string& path, Test* test)
{
  TestRunnerBase::BeforeTest(path, test);
  testOut.clear();
  std::string fullName = path + "/" + test->TestName();
  std::string cpath = ExtractTestPath(fullName, testLogLevel);
  if (cpath != pathPrev) {
    if (!pathPrev.empty()) {
      RunnerLog() << "  ";
      SummaryLog() << "  ";
      pathStats.TestSet().PrintShort(RunnerLog());
      pathStats.TestSet().PrintShort(SummaryLog());
      RunnerLog() << std::endl;
      SummaryLog() << std::endl;
    }
    pathStats.Clear();
    RunnerLog() << cpath << "  " << std::endl;
    SummaryLog() << cpath << "  " << std::endl;
    pathPrev = cpath;
  }
}

bool HTestRunner::BeforeTestSet(TestSet& testSet)
{
  time_t time_begin = time(0);
  tm* time_begin_UTC = gmtime(&time_begin);
  std::string testLogName = context->Opts()->GetString("testlog", "test.log");
  testLog.open(testLogName.c_str(), std::ofstream::out);
  if (!testLog.is_open()) {
    context->Error() << "Failed to open test log " << testLogName << std::endl;
    return false;
  }
  std::string testSummaryName = context->Opts()->GetString("testsummary", "test_summary.log");
  testSummary.open(testSummaryName.c_str(), std::ofstream::out);
  if (!testSummary.is_open()) {
    context->Error() << "Failed to open test summary " << testSummaryName << std::endl;
    return false;
  }
  RunnerLog() << "UTC Start Date & Time: " << asctime(time_begin_UTC) << std::endl;
  SummaryLog() << "UTC Start Date & Time: " << asctime(time_begin_UTC) << std::endl;
  if (context->Opts()->GetBoolean("dsign")) {
     SummaryLog() << "Digital Signature: " << "NNNNNNNNNNNNN" << std::endl << std::endl;
     testLog << "Digital Signature: " << "NNNNNNNNNNNNN" << std::endl << std::endl;
  }
  context->Runtime()->PrintInfo(SummaryLog());
  SummaryLog() << std::endl << std::endl;
  return true;
}

bool HTestRunner::AfterTestSet(TestSet& testSet)
{
  time_t time_end = time(0);
  tm* time_end_UTC = gmtime(&time_end);
  // Process first path.
  RunnerLog() << "  ";
  SummaryLog() << "  ";
  pathStats.TestSet().PrintShort(RunnerLog());
  pathStats.TestSet().PrintShort(SummaryLog());
  RunnerLog() << std::endl;
  SummaryLog() << std::endl;

  // Totals
  RunnerLog() << std::endl << "Testrun" << std::endl << "  ";
  SummaryLog() << std::endl << "Testrun" << std::endl << "  ";
  Stats().TestSet().PrintShort(RunnerLog()); RunnerLog() << std::endl;
  Stats().TestSet().PrintShort(SummaryLog()); SummaryLog() << std::endl;
  RunnerLog()  << std::endl << "UTC Finish Date & Time: " << asctime(time_end_UTC) << std::endl;
  SummaryLog() << std::endl << "UTC Finish Date & Time: " << asctime(time_end_UTC) << std::endl;
  if (context->Opts()->GetBoolean("dsign")) {
     SummaryLog() << "Digital Signature: " << "NNNNNNNNNNNNN" << std::endl;
     testLog << "Digital Signature: " << "NNNNNNNNNNNNN" << std::endl;
  }
  testLog.close();
  testSummary.close();
  return true;
}

void HTestRunner::AfterTest(const std::string& path, Test* test, const TestResult& result)
{
  if (!result.IsPassed() || context->IsVerbose("testlog", false)) {
    testLog << testOut.str();
  }
  std::string fullTestName = path + "/" + test->TestName();
  testLog <<
    result.StatusString() << ": " <<
    fullTestName << " " << std::setprecision(2) <<
    result.ExecutionTime() << "s" << std::endl;
  testLog << std::endl;
  result.IncStats(pathStats);
  TestRunnerBase::AfterTest(path, test, result);
  testOut.str(std::string());
}

}
