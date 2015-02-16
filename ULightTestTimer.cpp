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

#include "ULightTestTimer.h"
#include "ULightCpp.h"

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

namespace ULightCpp
{

static inline int64_t ToMicroSeconds(struct timespec& tm)
{
	return (tm.tv_nsec / 1000) + (tm.tv_sec * 1000000);
}

static inline bool portable_gettime(struct timespec& tm)
{
	#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
	struct timeval now;
    	int rv = gettimeofday(&now, NULL);
    	if (rv) return rv;
   	tm.tv_sec  = now.tv_sec;
   	tm.tv_nsec = now.tv_usec * 1000;
   	return true;
	
	#else
	return clock_gettime(CLOCK_REALTIME, &tm) >= 0;
	#endif
}

ULightTestTimer::ULightTestTimer() : m_loopCount(0), m_unitTests(nullptr)
{
	struct timespec tm;
	if (portable_gettime(tm))
		m_bad = false, m_pit = ToMicroSeconds(tm);
	else
		m_bad = true;
}

ULightTestTimer::ULightTestTimer(ULightTests *unitTests, int loopCount) : ULightTestTimer()
{
	m_loopCount = loopCount;
	m_unitTests = unitTests;
}

ULightTestTimer::~ULightTestTimer()
{
	if (m_unitTests != nullptr)
	{
		ULightTestInfo *testInfo = m_unitTests->GetCurrentTestInfo();
		testInfo->benchmarked = true;
		int64_t poll = Poll();
		testInfo->benchmarktime = poll;
		if (m_loopCount > 0)
			testInfo->itemsPerSecond = ((1000000.0 / ((double)poll)) * m_loopCount);
	}
}

int64_t ULightTestTimer::Poll()
{
	struct timespec tm;
	if (m_bad || !portable_gettime(tm))
		return 0;
	else
		return ToMicroSeconds(tm) - m_pit;
}

}
