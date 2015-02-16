/*
	Copyright 2015 Anthony Smith

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

#ifndef __ULightCpp__ULightTests__
#define __ULightCpp__ULightTests__

#include "ULightTestTimer.h"
#include "ULightCppThreadStarter.h"

#include <initializer_list>
#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <sstream>
#include <functional>
#include <mutex>

namespace ULightCpp
{

enum class ULightTestStatus { Inconclusive, Passed, Failed, Skipped, Incomplete };

struct ULightTestInfo
{
	ULightTestInfo(std::wstring testName_, std::function<void()> testFn_, bool stressTest_)
	 :	testName(testName_), testFn(testFn_),
		status(ULightTestStatus::Inconclusive), error(L""), filename(L""), lineNumber(0), ignore(false), stressTest(stressTest_), benchmarked(false), benchmarktime(0), itemsPerSecond(0)
		{}

    std::wstring testName;
	std::function<void()> testSetup;
	std::function<void()> testTeardown;
    std::function<void()> testFn;
	ULightTestThreadStarter threadStarter;
    
	ULightTestStatus status;
    std::wstring error;
    std::wstring filename;
    int lineNumber;
	bool ignore;
	bool stressTest;
    bool benchmarked;
    int64_t benchmarktime;
	int64_t itemsPerSecond;
};

class ULightTests
{
    public:
        ULightTests();
        virtual ~ULightTests();

		void AddTestSetup(std::wstring testName_, std::function<void()> testFn_);
		void AddTestTeardown(std::wstring testName_, std::function<void()> testFn_);
		void AddTask(std::wstring testName_, std::function<void()> testFn_, size_t count);
		void AddTest(std::wstring testName_, std::function<void()> testFn_, bool stressTest_);

		void Init(int argc, char **argv, std::wostream& ostr);
        void Execute();
        void ReportToStream();

		void ReportBack(const std::wstring& msg);

		void DirectToStream(const std::wstring& msg);

        ULightTestInfo *GetCurrentTestInfo();
    protected:
    private:
		std::wostream *outStream;
        std::vector<ULightTestInfo *> m_tests;
        std::vector<std::wstring> m_namedTests;
		std::deque<std::wstring> m_reportsBack;
        int64_t m_elapsedTime;
        bool m_benchmarks;
		bool m_reports;
        bool m_verbose;
		bool m_runStressTests;
        ULightTestInfo *m_currentTest;
};

enum ULightTestStage { Setup, Run, Teardown, Task };

class UnitTest
{
public:
    UnitTest(ULightTests& unitTests, std::function<void()> test, const std::wstring& testName, bool stressTest, ULightTestStage stage, size_t count)
    {
		if (stage == ULightTestStage::Setup)
			unitTests.AddTestSetup(testName, test);
		else if (stage == ULightTestStage::Task)
			unitTests.AddTask( testName, test, count);
		else if (stage == ULightTestStage::Run)
			unitTests.AddTest( testName, test, stressTest );
		else if (stage == ULightTestStage::Teardown)
			unitTests.AddTestTeardown(testName,	test);
    }
};

class UnitTestException
{
public:
	std::wstring error;
	std::wstring filename;
	int lineNumber;

	UnitTestException(std::wstring error_, std::wstring filename_, int lineNumber_)
		: error(error_), filename(FixFileName(filename_)), lineNumber(lineNumber_)
	{

    }
private:
	static std::wstring FixFileName(const std::wstring filename);
};

class UnitTestSkipException
{
public:
	UnitTestSkipException() {}
};

class UnitTestIncompleteException
{
public:
	UnitTestIncompleteException() {}
};

ULightTests& GetTestHarness();

#define IMPLEMENT_UNITTESTS(wstream) \
int main(int argc, char **argv)\
{\
	ULightCpp::GetTestHarness().Init(argc, argv, wstream); \
    ULightCpp::GetTestHarness().Execute();\
    ULightCpp::GetTestHarness().ReportToStream();\
    return 0;\
}

#define UNITTEST_WIDEN2(x) L ## x
#define UNITTEST_WIDEN(x) UNITTEST_WIDEN2(x)

#define UNITTESTFILE \
    extern ULightCpp::ULightTests unitTests;

#define SETUP(testName) \
    static void Test##testName##_Setup(); \
    static ULightCpp::UnitTest impl_setup_##testName(ULightCpp::GetTestHarness(), Test##testName##_Setup, UNITTEST_WIDEN(#testName), false, ULightCpp::ULightTestStage::Setup, 0); \
    static void Test##testName##_Setup()

#define TEARDOWN(testName) \
    static void Test##testName##_Teardown(); \
    static ULightCpp::UnitTest impl_teardown_##testName(ULightCpp::GetTestHarness(), Test##testName##_Teardown, UNITTEST_WIDEN(#testName), false, ULightCpp::ULightTestStage::Teardown, 0); \
    static void Test##testName##_Teardown()

#define TEST(testName) \
    static void Test##testName(); \
    static ULightCpp::UnitTest impl_##testName(ULightCpp::GetTestHarness(), Test##testName, UNITTEST_WIDEN(#testName), false, ULightCpp::ULightTestStage::Run, 0); \
    static void Test##testName()

#define TEST_TASK(testName, subName, count) \
    static void Test##testName##task##subName(); \
    static ULightCpp::UnitTest impl_##testName##task##subName(ULightCpp::GetTestHarness(), Test##testName##task##subName, UNITTEST_WIDEN(#testName), false, ULightCpp::ULightTestStage::Task, count); \
    static void Test##testName##task##subName()

#define STRESSTEST(testName) \
    static void Test##testName(); \
    static ULightCpp::UnitTest impl_##testName(ULightCpp::GetTestHarness(), Test##testName, UNITTEST_WIDEN(#testName), true, ULightCpp::ULightTestStage::Run, 0); \
    static void Test##testName()

#define SKIPTEST throw ULightCpp::UnitTestSkipException();

#define INCOMPLETE throw ULightCpp::UnitTestIncompleteException();

#define T(pred, msg) \
{ \
	if (!(pred)) \
	{ \
		std::wstringstream str_dee5e24c44b011e38782089e0125ab67; \
		str_dee5e24c44b011e38782089e0125ab67 << msg; \
		throw ULightCpp::UnitTestException(str_dee5e24c44b011e38782089e0125ab67.str(), UNITTEST_WIDEN(__FILE__), __LINE__); \
	} \
}

#define REPORT(msg) \
{ \
	std::wstringstream str_dee5e24c44b011e38782089e0125ab67; \
	str_dee5e24c44b011e38782089e0125ab67 << msg; \
	ULightCpp::GetTestHarness().ReportBack(str_dee5e24c44b011e38782089e0125ab67.str()); \
}

#define DIRECT(msg) \
{ \
	std::wstringstream str_dee5e24c44b011e38782089e0125ab67; \
	str_dee5e24c44b011e38782089e0125ab67 << msg; \
	ULightCpp::GetTestHarness().DirectToStream(str_dee5e24c44b011e38782089e0125ab67.str()); \
}

#define EXPECT_EXCEPTION(ExType, Code, msg) \
{ \
	bool good = false; \
	try \
	{ \
		{ Code; } \
		good = false; \
	} \
	catch(ExType ex) \
	{ \
		good = true; \
	} \
	catch(...) \
	{ \
		good = false; \
	} \
	T(good, msg); \
}

#define BENCHMARK ULightCpp::ULightTestTimer timer_dee5e24c44b011e38782089e0125ab67(&ULightCpp::GetTestHarness(), 0);

#define BENCHIPS(ItemsPerSecond) ULightCpp::ULightTestTimer timer_dee5e24c44b011e38782089e0125ab67(&ULightCpp::GetTestHarness(), ItemsPerSecond);

}

#endif // __ULightCpp__ULightTests__