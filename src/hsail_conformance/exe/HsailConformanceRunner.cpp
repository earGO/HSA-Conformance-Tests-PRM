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

#include "Options.hpp"
#include "HexlTestFactory.hpp"
#include "HexlTestRunner.hpp"
#include <iostream>
#include <memory>
#include "HexlResource.hpp"

#include "PrmCoreTests.hpp"
#include "SysArchMandatoryTests.hpp"
#include "ImagesTests.hpp"
#include "HexlLib.hpp"
#ifdef ENABLE_HEXL_AGENT
#include "HexlAgent.hpp"
#endif // ENABLE_HEXL_AGENT
#include "CoreConfig.hpp"
#include "SignalTests.hpp"
#include "Brig.h"

using namespace hexl;
using namespace hexl::emitter;

namespace hsail_conformance {

DECLARE_TESTSET_UNION(PrmTests);

PrmTests::PrmTests()
  : TestSetUnion("prm")
{
    Add(NewPrmCoreTests());
    Add(NewPrmImagesTests());
}

DECLARE_TESTSET_UNION(SysArchTests);

SysArchTests::SysArchTests()
  : TestSetUnion("sysarch")
{
    Add(NewSysArchMandatoryTests());
}

DECLARE_TESTSET_UNION(HSATests);

HSATests::HSATests()
  : TestSetUnion("")
{
    Add(new PrmTests());
    Add(new SysArchTests());
}

class HCTestFactory : public DefaultTestFactory {
private:
  Context* context;
  hexl::TestSetUnion* hsaTests;

public:
  HCTestFactory(Context* context_)
    : context(context_), hsaTests(new HSATests())
  {
  }

  ~HCTestFactory()
  {
    delete hsaTests;
  }

  virtual Test* CreateTest(const std::string& type, const std::string& name, const Options& options = Options())
  {
    assert(false);
    return 0;
  }

  virtual TestSet* CreateTestSet(const std::string& type)
  {
    TestSet* ts;
      ts = hsaTests;
      ts->InitContext(context);
      if (type != "all") {
        TestNameFilter* filter = new TestNameFilter(type);
        TestSet* fts = hsaTests->Filter(filter);
        if (fts != ts) {
          fts->InitContext(context);
          ts = fts;
        }
      }

    return ts;
  }
};

class HCRunner {
public:
  HCRunner(int argc_, char **argv_)
    : argc(argc_), argv(argv_), context(new Context()),
      testFactory(new HCTestFactory(context.get())), runner(0),
      coreConfig(0)
  {
    context->Put("hexl.log.stream.debug", &std::cout);
    context->Put("hexl.log.stream.info", &std::cout);
    context->Put("hexl.log.stream.error", &std::cout);
  }
  ~HCRunner()
  { 
      delete testFactory; 
      delete coreConfig;
  }

  void Run();

private:
  int argc;
  char **argv;
  std::unique_ptr<Context> context;
  Options options;
  TestFactory* testFactory;
  TestRunner* runner;
  CoreConfig* coreConfig;
  TestRunner* CreateTestRunner();
  TestSet* CreateTestSet();
};

TestRunner* HCRunner::CreateTestRunner()
{
  std::string runner = options.GetString("runner");
#ifdef ENABLE_HEXL_AGENT
  if (options.IsSet("remote")) {
    if (options.IsSet("runner") && runner != "remote") {
      std::cout << "Runner should be set to remote for -remote" << std::endl;
      exit(20);
    }
    RemoteTestRunner* remoteTestRunner = new RemoteTestRunner(context, options.GetString("remote"));
    if (!remoteTestRunner->Connect()) {
      exit(19);
    }
    return remoteTestRunner;
  } else
#endif // ENABLE_HEXL_AGENT
  if (runner == "hrunner" || runner.empty()) {
    return new HTestRunner(context.get());
  } else if (runner == "simple") {
    return new SimpleTestRunner(context.get());
  } else {
    std::cout << "Unsupported runner: " << runner << std::endl;
    exit(20);
  }
}

TestSet* HCRunner::CreateTestSet()
{
  TestSet* ts = testFactory->CreateTestSet(options.GetString("tests"));
  ts->InitContext(context.get());
  if (options.IsSet("exclude")) {
    ExcludeListFilter* filter = new ExcludeListFilter();
    filter->Load(context->RM(), options.GetString("exclude"));
    TestSet* fts = filter->Filter(ts);
    if (fts != ts) {
      ts->InitContext(context.get());
      ts = fts;
    }
  }
  return ts;
}

void HCRunner::Run()
{
  std::cout <<
    "HSA Conformance" <<
    " (" <<
    "HSAIL " << BRIG_VERSION_HSAIL_MAJOR << "." << BRIG_VERSION_HSAIL_MINOR <<
    ", BRIG " << BRIG_VERSION_BRIG_MAJOR << "." << BRIG_VERSION_BRIG_MINOR <<
    ")" << std::endl;
  OptionRegistry optReg;
  optReg.RegisterOption("rt");
  optReg.RegisterOption("runner");
  optReg.RegisterOption("remote");
  optReg.RegisterOption("testbase");
  optReg.RegisterOption("results");
  optReg.RegisterOption("tests");
  optReg.RegisterOption("testloglevel");
  optReg.RegisterOption("testlog");
  optReg.RegisterOption("testsummary");
  optReg.RegisterOption("rtlib");
  optReg.RegisterOption("exclude");
  optReg.RegisterBooleanOption("dummy");
  optReg.RegisterBooleanOption("verbose");
  optReg.RegisterBooleanOption("dump");
  optReg.RegisterBooleanOption("dump.brig");
  optReg.RegisterBooleanOption("dump.hsail");
  optReg.RegisterBooleanOption("dump.dispatchsetup");
  optReg.RegisterBooleanOption("noFtzF16");
  optReg.RegisterBooleanOption("dsign");
  optReg.RegisterOption("match");
  optReg.RegisterOption("timeout");
  optReg.RegisterOption("profile");
  {
    int n = hexl::ParseOptions(argc, argv, optReg, options);
    if (n != 0) {
      std::cout << "Invalid option: " << argv[n] << std::endl;
      exit(4);
    }
    if (!options.IsSet("tests")) {
      std::cout << "tests option is not set" << std::endl;
      exit(5);
    }
    std::string match = options.GetString("match", "");
    if (match.length() > 0 && match[0] == '!' && match.length() <= 1) {
      std::cout << "Bad -match: '" << match << "'" << std::endl;
      exit(6);
    }
    std::string profile = options.GetString("profile");
    if (profile.length() > 0 && profile != "full" && profile != "base") {
      std::cout << "Invalid profile option: '" << profile << "'" << std::endl;
      exit(7);
    }
  }
  context->Move("hexl.stats", new AllStats());
  ResourceManager* rm = new DirectoryResourceManager(options.GetString("testbase", "."), options.GetString("results", "."));
  context->Put("hexl.rm", rm);
  context->Put("hexl.options", &options);
  runtime::RuntimeContext* runtime = 0;
  runtime = CreateRuntimeContext(context.get());
  if (!runtime) {
    std::cout << "Failed to create runtime" << std::endl;
    exit(8);
  }
  context->Put("hexl.runtime", runtime);
  context->Put("hexl.testFactory", testFactory);

  coreConfig = CoreConfig::CreateAndInitialize(context.get());
  context->Put(CoreConfig::CONTEXT_KEY, coreConfig);

  runner = CreateTestRunner();
  TestSet* tests = CreateTestSet();
  assert(tests);
  runner->RunTests(*tests);

  // cleanup in reverse order. new never fails.
  delete runner; runner = 0;
  if (runtime) { delete runtime; } 
  delete rm;
}

}

int main(int argc, char **argv)
{
#ifdef _WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif // _WIN32
  hsail_conformance::HCRunner hcr(argc, argv);
  hcr.Run();
  return 0;
}
