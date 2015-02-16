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

#include "ULightCpp.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <sys/time.h>
#include <sys/times.h>

namespace ULightCpp
{

ULightTests& GetTestHarness()
{
	static ULightTests unitTests;
	return unitTests;
}

static std::wstring DateAsString()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm brokenDownTime {0};
	auto ret = localtime_r(&tv.tv_sec, &brokenDownTime);
	if (ret != nullptr)
	{
		std::wstringstream str;
		str << std::setfill(L'0')
			<< std::setw(4) << brokenDownTime.tm_year+1900 << L"-"
			<< std::setw(2) << brokenDownTime.tm_mon+1 << L"-"
			<< std::setw(2) << brokenDownTime.tm_mday;
		return str.str();
	}
	else
	{
		return L"Time convert error";
	}
}

static std::wstring TimeAsString()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm brokenDownTime {0};
	auto ret = localtime_r(&tv.tv_sec, &brokenDownTime);
	if (ret != nullptr)
	{
		std::wstringstream str;
		str << std::setfill(L'0') << std::setw(2) << brokenDownTime.tm_hour << L":"
			<< std::setfill(L'0') << std::setw(2) << brokenDownTime.tm_min << L":"
			<< std::setfill(L'0') << std::setw(2) << brokenDownTime.tm_sec;
		return str.str();
	}
	else
	{
		return L"Time convert error";
	}
}

static std::wstring MakeNumberPrettyNumber(int64_t val)
{
	std::wstringstream sstr;
	sstr << val;
	std::wstring s = sstr.str();
	int pos = (int)(s.length() - 1);
	int cnt = 3;
	while(pos >= 0)
	{
		--cnt;
		if (cnt == 0 && pos > 0)
		{
			cnt = 3;
			s.insert(s.begin() + pos, L',');
		}
		--pos;
	}
	return s;
}

ULightTests::ULightTests()
 : outStream(nullptr), m_elapsedTime(0), m_benchmarks(false), m_reports(false), m_verbose(false), m_runStressTests(false), m_currentTest(nullptr)
{
    //ctor
}

ULightTests::~ULightTests()
{
	for (auto testInfo : m_tests)
	{
		delete testInfo;
	}
    //dtor
}

static ULightTestInfo *FindOrCreateTestInfo(std::vector<ULightTestInfo *>& m_tests, std::wstring testName_)
{
	auto ret = std::find_if(m_tests.begin(), m_tests.end(), [&](ULightTestInfo *i) { return i->testName == testName_; } );
	if (ret != m_tests.end())
		return *ret;
	ULightTestInfo *uti = new ULightTestInfo(testName_, nullptr, false);
	m_tests.push_back(uti);
	return uti;
}

void ULightTests::AddTestSetup(std::wstring testName_, std::function<void()> testFn_)
{
	ULightTestInfo *testInfo = FindOrCreateTestInfo(m_tests, testName_);
	testInfo->testSetup = testFn_;
}

void ULightTests::AddTestTeardown(std::wstring testName_, std::function<void()> testFn_)
{
	ULightTestInfo *testInfo = FindOrCreateTestInfo(m_tests, testName_);
	testInfo->testTeardown = testFn_;
}

void ULightTests::AddTask(std::wstring testName_, std::function<void()> testFn_, size_t count)
{
	ULightTestInfo *testInfo = FindOrCreateTestInfo(m_tests, testName_);
	testInfo->threadStarter.add(testFn_, count);
}

void ULightTests::AddTest(std::wstring testName_, std::function<void()> testFn_, bool stressTest_)
{
	ULightTestInfo *testInfo = FindOrCreateTestInfo(m_tests, testName_);
	testInfo->testFn = testFn_;
	testInfo->stressTest = stressTest_;
}

static void RunTestFn(ULightTestStage stage, ULightTestInfo& testInfo, bool runStressTests)
{
    try
    {
		if (!runStressTests && testInfo.stressTest)
			throw UnitTestSkipException();
		if (stage == ULightTestStage::Setup && testInfo.testSetup)
		{
			testInfo.testSetup();
		}
		else if (stage == ULightTestStage::Task && testInfo.threadStarter.has_tasks())
		{
			ULightRunResults results = testInfo.threadStarter.run();
			if (results.failed > 0)
				testInfo.status = ULightTestStatus::Failed;
			else if (results.incomplete > 0)
				testInfo.status = ULightTestStatus::Incomplete;
			else if (results.skipped > 0)
				testInfo.status = ULightTestStatus::Skipped;
			auto errors = results.errors;
			if (errors.size() > 0)
			{
				std::wstringstream str;
				str << "Error occurred in " << errors[0].second << L" threads: "
					<< errors[0].first;
				testInfo.error = str.str();
			}
		}
		else if (stage == ULightTestStage::Run && testInfo.status == ULightTestStatus::Inconclusive)
		{
			if (testInfo.testFn)
			{
				testInfo.testFn();
				testInfo.status = ULightTestStatus::Passed;
			}
			else if (testInfo.threadStarter.has_tasks())
			{
				testInfo.status = ULightTestStatus::Passed;
			}
		}
		else if (stage == ULightTestStage::Teardown && testInfo.testTeardown)
		{
			testInfo.testTeardown();
		}
    }
    catch(UnitTestException ex)
    {
		testInfo.status = ULightTestStatus::Failed;
        testInfo.error = ex.error;
        testInfo.filename = ex.filename;
        testInfo.lineNumber = ex.lineNumber;
    }
	catch(UnitTestSkipException skipEx)
	{
		testInfo.status = ULightTestStatus::Skipped;
	}
	catch(UnitTestIncompleteException incEx)
	{
		testInfo.status = ULightTestStatus::Incomplete;
	}
    catch(...)
    {
		testInfo.status = ULightTestStatus::Failed;
        testInfo.error = L"Unexpected exception";
        testInfo.filename = L"";
        testInfo.lineNumber = 0;
    }
}

static void RunTest(ULightTestInfo& testInfo, bool runStressTests)
{
    //std::wcout << L"Running " << testInfo.testName << std::endl;

	RunTestFn(ULightTestStage::Setup, testInfo, runStressTests);
	RunTestFn(ULightTestStage::Task, testInfo, runStressTests);
	RunTestFn(ULightTestStage::Run, testInfo, runStressTests);
	RunTestFn(ULightTestStage::Teardown, testInfo, runStressTests);
}

ULightTestInfo *ULightTests::GetCurrentTestInfo()
{
	return m_currentTest;
}

void ULightTests::Init(int argc, char **argv, std::wostream& ostr)
{
	outStream = &ostr;
	std::vector<std::string> args;
	std::copy(argv + 1, argv + argc, std::back_inserter(args));

	for(auto& s : args)
	{
		std::wstringstream str;
		str << s.c_str();
		std::wstring arg = str.str();

		if (arg == L"-b" || arg == L"--benchmark")
			m_benchmarks = true;
		else if (arg == L"-v" || arg == L"--verbose")
			m_verbose = true;
		else if (arg == L"-s" || arg == L"--stress")
			m_runStressTests = true;
		else if (arg == L"-r" || arg == L"--reports")
			m_reports = true;
		else if (arg.length() > 0 && arg[0] != L'-')
			m_namedTests.push_back(arg);
	}
}

void ULightTests::Execute()
{
	ULightTestTimer timer;
	bool namedOnly = m_namedTests.size() > 0;
    for(auto& testInfo : m_tests)
    {
		m_currentTest = testInfo;
		if (namedOnly)
		{
			if (std::find(m_namedTests.begin(), m_namedTests.end(), testInfo->testName) != m_namedTests.end())
				RunTest(*testInfo, m_runStressTests);
			else
				testInfo->ignore = true;
		}
		else
		{
        	RunTest(*testInfo, m_runStressTests);
		}
        m_currentTest = nullptr;
    }
	m_elapsedTime = timer.Poll();
}

void ULightTests::ReportBack(const std::wstring& msg)
{
	std::wstringstream ss;
	ss << m_currentTest->testName << L":" << std::endl
		<< L" " << msg;
	m_reportsBack.push_back(ss.str());
}

void ULightTests::ReportToStream()
{
	if (outStream == nullptr)
		return;
	std::wostream& os(*outStream);
	int total = 0;
	int passed = 0;
	int skipped = 0;
	int incomplete = 0;
	int failed = 0;
	for(auto& testInfo : m_tests)
	{
		if (!testInfo->ignore)
		{
			++total;
			if (testInfo->status == ULightTestStatus::Passed)
				++passed;
			else if (testInfo->status == ULightTestStatus::Skipped)
				++skipped;
			else if (testInfo->status == ULightTestStatus::Incomplete)
				++incomplete;
			else
				++failed;
		}
	}
	os << std::endl;

	if (m_benchmarks)
	{
		for (auto& testInfo : m_tests)
		{
			if (testInfo->ignore)
				continue;
			if (testInfo->benchmarked)
			{
				os << std::setw(8) << MakeNumberPrettyNumber(testInfo->benchmarktime) << L"us ";
				if (testInfo->itemsPerSecond > 0)
					os << std::setw(12) << MakeNumberPrettyNumber(testInfo->itemsPerSecond) << L"/s ";
				else
					os << std::setw(12) << L"" << L"   ";
				os << testInfo->testName << std::endl;
			}
		}
		os << std::endl;
	}

	if (m_reports && m_reportsBack.size() > 0)
	{
		for (auto &rep : m_reportsBack)
		{
			os << rep << std::endl;
		}
		os << std::endl;
	}

	if (m_verbose)
	{
		for(auto& testInfo : m_tests)
		{
			if (testInfo->ignore)
				continue;
			if (testInfo->status == ULightTestStatus::Passed)
				os << testInfo->testName << L" : passed" << std::endl;
			else if (testInfo->status == ULightTestStatus::Skipped)
				os << testInfo->testName << L" : skipped" << std::endl;
			else if (testInfo->status == ULightTestStatus::Incomplete)
				os << testInfo->testName << L" : incomplete" << std::endl;
		}
		os << std::endl;
	}

	for(auto& testInfo : m_tests)
	{
		if (testInfo->ignore)
			continue;
		if (testInfo->status == ULightTestStatus::Failed)
		{
			os << L"Test Failed: " << testInfo->testName << std::endl
				<< L" Location: " << testInfo->filename << L" (" << testInfo->lineNumber << L")" << std::endl
				<< L" Error: " << testInfo->error << std::endl << std::endl;
		}
	}

	os << L"Results (" << DateAsString() << L" " << TimeAsString() << "): " << std::endl
		<< L" Passed       " << passed << std::endl
		<< L" Failed       " << failed << std::endl
		<< L" Skipped      " << skipped << std::endl
		<< L" Incomplete   " << incomplete << std::endl
		<< L" Total        " << total << std::endl
		<< L" Elapsed      " << MakeNumberPrettyNumber(m_elapsedTime) << "us" << std::endl
		<< L" Benchmarking " << (m_benchmarks ? L"Enabled" : L"Disabled") << std::endl
		<< std::endl;
}

void ULightTests::DirectToStream(const std::wstring& msg)
{
	if (outStream != nullptr)
		*outStream << msg << std::endl;
}

std::wstring UnitTestException::FixFileName(const std::wstring filename)
{
	if (filename.length() == 0)
		return filename;
	size_t pos = filename.find_last_of('/');
	if (pos == std::wstring::npos || pos == (filename.length() - 1))
		return filename;
	return filename.substr(pos+1);
}

}
