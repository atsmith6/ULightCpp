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

#ifndef __ULightCpp__ULightCppThreadStarter__
#define __ULightCpp__ULightCppThreadStarter__

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <map>

namespace ULightCpp
{

struct ULightRunResults
{
	size_t passed;
	size_t failed;
	size_t skipped;
	size_t incomplete;
	std::vector<std::pair<std::wstring, size_t>> errors;
};

class ULightTestThreadInfo
{
	std::mutex m_mutex;
	size_t m_passed;
	size_t m_failed;
	size_t m_skipped;
	size_t m_incomplete;
	std::map<std::wstring, size_t> m_errors;
public:
	ULightTestThreadInfo();
	~ULightTestThreadInfo();
	
	size_t get_passed();
	size_t get_failed();
	size_t get_skipped();
	size_t get_incomplete();
	
	std::vector<std::pair<std::wstring, size_t>> get_errors();
	
	void set_passed();
	void set_incomplete();
	void set_skipped();
	void set_failed(const std::wstring& error);
};

class ULightTestThreadStarter
{
	std::vector<std::function<void()>> m_tasks;
	std::vector<std::thread> m_threads;
public:
	void add(std::function<void()> func, size_t count);
	ULightRunResults run();
	
	bool has_tasks();
};


} // namespace ULightCpp

#endif // __ULightCpp__ULightCppThreadStarter__
