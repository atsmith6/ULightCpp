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

#include "ULightCppThreadStarter.h"
#include "ULightCpp.h"

#include <thread>

namespace ULightCpp
{

ULightTestThreadInfo::ULightTestThreadInfo()
: m_passed(0), m_failed(0), m_skipped(0), m_incomplete(0)
{
}

ULightTestThreadInfo::~ULightTestThreadInfo()
{
}

size_t ULightTestThreadInfo::get_passed()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	return m_passed;
}

size_t ULightTestThreadInfo::get_failed()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	return m_failed;
}

size_t ULightTestThreadInfo::get_skipped()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	return m_skipped;
}

size_t ULightTestThreadInfo::get_incomplete()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	return m_incomplete;
}

std::vector<std::pair<std::wstring, size_t>> ULightTestThreadInfo::get_errors()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	
	std::vector<std::pair<std::wstring, size_t>> ret;
	for(auto it = m_errors.begin(); it != m_errors.end(); ++it)
	{
		ret.push_back(*it);
	}
	return std::move(ret);
}

void ULightTestThreadInfo::set_passed()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	
	m_passed += 1;
}

void ULightTestThreadInfo::set_skipped()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	
	m_skipped += 1;
}

void ULightTestThreadInfo::set_incomplete()
{
	std::lock_guard<std::mutex> lck { m_mutex };
	
	m_incomplete += 1;
}


void ULightTestThreadInfo::set_failed(const std::wstring& error)
{
	std::lock_guard<std::mutex> lck { m_mutex };

	m_failed += 1;
	auto it = m_errors.find(error);
	if (it == m_errors.end())
		m_errors.emplace(error, 1);
	else
		m_errors[error] += 1;
}


static void thread_proc(std::function<void()> func, ULightTestThreadInfo* info)
{
	try
	{
		func();
    }
    catch(UnitTestException ex)
    {
		info->set_failed(ex.error);
    }
	catch(UnitTestSkipException skipEx)
	{
		info->set_skipped();
	}
	catch(UnitTestIncompleteException incEx)
	{
		info->set_incomplete();
	}
    catch(...)
    {
		info->set_failed(L"Unexpected exception");
    }
}

void ULightTestThreadStarter::add(std::function<void()> func, size_t count)
{
	for(size_t i = 0; i < count; ++i)
	{
		m_tasks.push_back(func);
	}
}

ULightRunResults ULightTestThreadStarter::run()
{
	ULightTestThreadInfo info;
	
	for(auto& task : m_tasks)
	{
		m_threads.push_back(std::move(std::thread(thread_proc, task, &info)));
	}
	for(auto& thread : m_threads)
	{
		thread.join();
	}
	m_threads.clear();
	
	ULightRunResults results;
	results.passed = info.get_passed();
	results.failed = info.get_failed();
	results.skipped = info.get_skipped();
	results.incomplete = info.get_incomplete();
	results.errors = std::move(info.get_errors());
	
	return results;
}

bool ULightTestThreadStarter::has_tasks()
{
	return m_tasks.size() > 0;
}

} // namespace ULightCpp


