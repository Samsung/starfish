/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*!
 * \file
 * \brief CCTime class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#if defined (_WIN32)

#include <windows.h>
#include <time.h>

#pragma comment(lib, "Winmm.lib")

#elif defined (_LINUX)

#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#endif




#include <CSP.h>




#if defined (_WIN32)

#if !defined (_WIN32_REAL_CLOCK)

long	g_Clock_Delta = 0;

#endif

// Time value structure for Win32 platform
struct _CTTime
{
    unsigned long  time;            // seconds
    unsigned short millitm;         // milliseconds
    short          timezone;        // difference in minutes, moving westward, between UTC and local time.
    short          dstflag;         // nonzero if Daylight Saving Time is currently in effect for the local time zone.
};

extern "C" void _ftime(_CTTime* time);

#endif

#if defined (_LINUX)

pthread_mutex_t TimeMutex = PTHREAD_MUTEX_INITIALIZER;

#endif




void CCTime::Sleep(unsigned long msec)
{
#if defined (_WIN32)

	::Sleep((DWORD)msec);

#elif defined (_LINUX)

	struct timespec time;
	time.tv_sec  = (time_t)(msec / 1000);
	time.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&time, NULL);

#endif
}




unsigned long CCTime::Tick(void)
{
#if defined (_LINUX)

	clock_t ret;

	while ((ret = times(NULL)) == (clock_t)-1)
	{
	}

	long conf = sysconf(_SC_CLK_TCK);

//	return (unsigned long)ret / sysconf(_SC_CLK_TCK) * 1000;
	return (conf > 1000) ? (unsigned long) ret * 1000 / conf : (unsigned long) ret * (1000 / conf) ;

#elif defined (_WIN32)

	return (unsigned long)timeGetTime();

#endif
}




// this structure is just the local version of Visual C tm structure in time.h
struct tm_struct {
	int tm_sec;     // seconds after the minute - [0,59]
	int tm_min;     // minutes after the hour - [0,59]
	int tm_hour;    // hours since midnight - [0,23]
	int tm_mday;    // day of the month - [1,31]
	int tm_mon;     // months since January - [0,11]
	int tm_year;    // years since 1900
	int tm_wday;    // days since Sunday - [0,6]
	int tm_yday;    // days since January 1 - [0,365]
	int tm_isdst;   // daylight savings time flag  // NOT USED
};




// this array represents the number of days in one non-leap year at 
// the beginning of each month
unsigned long DaysToMonth[13] = 
{
	0,31,59,90,120,151,181,212,243,273,304,334,365
};




unsigned long DS1371_DateToBinary(tm_struct* datetime) 
{
	unsigned long iday;
	unsigned long val;

	iday = 365 * (datetime->tm_year - 70) + DaysToMonth[datetime->tm_mon] + (datetime->tm_mday - 1);
	iday = iday + (datetime->tm_year - 69) / 4;
	if ((datetime->tm_mon > 1) && ((datetime->tm_year % 4) == 0)) {
		iday++;
	}
	val = datetime->tm_sec + 60 * datetime->tm_min + 3600 * (datetime->tm_hour + 24 * iday);
	return val;
}




void DS1371_BinaryToDate(unsigned long binary, tm_struct* datetime)
{
	unsigned long hour;
	unsigned long day;
	unsigned long minute;
	unsigned long second;
	unsigned long month;
	unsigned long year;
	
	unsigned long whole_minutes;
	unsigned long whole_hours;
	unsigned long whole_days;
	unsigned long whole_days_since_1968;
	unsigned long leap_year_periods;
	unsigned long days_since_current_lyear;
	unsigned long whole_years;
	unsigned long days_since_first_of_year;
	unsigned long days_to_month;
	unsigned long day_of_week;
	
	whole_minutes = binary / 60;
	second = binary - (60 * whole_minutes);				// leftover seconds

	whole_hours  = whole_minutes / 60;
	minute = whole_minutes - (60 * whole_hours);		// leftover minutes

	whole_days   = whole_hours / 24;
	hour         = whole_hours - (24 * whole_days);		// leftover hours
	
	whole_days_since_1968 = whole_days + 365 + 366;
	leap_year_periods = whole_days_since_1968 / ((4 * 365) + 1);

	days_since_current_lyear = whole_days_since_1968 % ((4 * 365) + 1);
	
	// if days are after a current leap year then add a leap year period
	if ((days_since_current_lyear >= (31 + 29))) {
		leap_year_periods++;
	}
	whole_years = (whole_days_since_1968 - leap_year_periods) / 365;
	days_since_first_of_year = whole_days_since_1968 - (whole_years * 365) - leap_year_periods;

	if ((days_since_current_lyear <= 365) && (days_since_current_lyear >= 60)) {
		days_since_first_of_year++;
	}
	year = whole_years + 68;	

	// setup for a search for what month it is based on how many days have past
	//   within the current year
	month = 13;
	days_to_month = 366;
	while (days_since_first_of_year < days_to_month) {
		month--;
		days_to_month = DaysToMonth[month];
		if ((month >= 2) && ((year % 4) == 0)) {
			days_to_month++;
		}
	}
	day = days_since_first_of_year - days_to_month + 1;

	day_of_week = (whole_days  + 4) % 7;

	datetime->tm_yday = 
		days_since_first_of_year;		/* days since January 1 - [0,365]		*/
	datetime->tm_sec  = second;			/* seconds after the minute - [0,59]	*/
	datetime->tm_min  = minute;			/* minutes after the hour - [0,59]		*/
	datetime->tm_hour = hour;			/* hours since midnight - [0,23]		*/
	datetime->tm_mday = day;			/* day of the month - [1,31]			*/
	datetime->tm_wday = day_of_week;	/* days since Sunday - [0,6]			*/
	datetime->tm_mon  = month;			/* months since January - [0,11]		*/
	datetime->tm_year = year;			/* years since 1900						*/

	return;
}




void CCTime::ConvertEpochToLocalTime(CTData* localtime, unsigned long sec, long timezone, bool daylight)
{
	ASSERT(localtime != NULL);

	sec += timezone * 60 + (daylight ? 3600 : 0);

	tm_struct date;
	DS1371_BinaryToDate(sec, &date);

	localtime->year   = date.tm_year + 1900;
	localtime->month  = date.tm_mon + 1;
	localtime->day    = date.tm_mday;
	localtime->hour   = date.tm_hour;
	localtime->minute = date.tm_min;
	localtime->second = date.tm_sec;

	return;
}




void CCTime::ConvertLocalToEpochTime(unsigned long* sec, const CTData* localtime, long timezone, bool daylight)
{
	ASSERT(sec != NULL);
	ASSERT(localtime != NULL);

	tm_struct date;
	date.tm_year = localtime->year - 1900;
	date.tm_mon  = localtime->month - 1;
	date.tm_mday = localtime->day;
	date.tm_hour = localtime->hour;
	date.tm_min  = localtime->minute;
	date.tm_sec  = localtime->second;

	*sec = DS1371_DateToBinary(&date);
	*sec -= timezone * 60 + (daylight ? 3600 : 0);

	return;
}




unsigned long CCTime::EpochTime(void)
{
#if defined (_WIN32)

#if !defined (_WIN32_REAL_CLOCK)

	_CTTime time;
	_ftime(&time);
	

	HKEY key;
	DWORD dwDisp;
	DWORD Size;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\CSP\\OSAL" ,0,NULL,REG_OPTION_NON_VOLATILE, KEY_READ,NULL,&key,&dwDisp) !=ERROR_SUCCESS) 
		return 0;

	Size = sizeof(LONG);

	if (RegQueryValueEx(key, "CCTIME_CLOCK_DELTA", 0, NULL,(LPBYTE)&g_Clock_Delta, &Size) != ERROR_SUCCESS)
		g_Clock_Delta = 0;
	RegCloseKey(key);


	return time.time + g_Clock_Delta;

#else

	_CTTime time;
	_ftime(&time);

	return time.time;

#endif

#elif defined (_LINUX)

	struct timeval time;
	gettimeofday(&time, NULL);

	return (unsigned long)time.tv_sec;

#endif
}



void CCTime::SetEpochTime(unsigned long sec)
{
#if defined (_WIN32)

#if defined (_WIN32_WCE)
	TIME_ZONE_INFORMATION info;
	GetTimeZoneInformation(&info);

	tm_struct tm;
	DS1371_BinaryToDate(sec - info.Bias * 60, &tm);
	SYSTEMTIME time = {tm.tm_year + 1900, tm.tm_mon+1, tm.tm_wday, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, 0};

	SetLocalTime(&time);

#else

#if !defined (_WIN32_REAL_CLOCK)

	_CTTime time;
	_ftime(&time);

	g_Clock_Delta = sec - time.time;


	HKEY key;
	DWORD dwDisp;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\CSP\\OSAL",0,NULL,REG_OPTION_NON_VOLATILE, KEY_WRITE,NULL,&key,&dwDisp) !=ERROR_SUCCESS)
		return;

	if (RegSetValueEx(key, "CCTIME_CLOCK_DELTA",0,REG_DWORD,(LPBYTE)&g_Clock_Delta , sizeof(UINT)) !=ERROR_SUCCESS)
		return;

	RegCloseKey(key);

#else

	CTData utc;

	ConvertEpochToLocalTime(&utc, sec, 0, false);
	SYSTEMTIME systime;

	systime.wYear = utc.year;
	systime.wMonth = utc.month;
	systime.wDay = utc.day;
	systime.wHour = utc.hour;
	systime.wMinute = utc.minute;
	systime.wSecond = utc.second;
	systime.wMilliseconds = 0;

	HANDLE hToken;				// process token
	TOKEN_PRIVILEGES tp;		// token provileges
	TOKEN_PRIVILEGES oldtp;		// old token privileges
	DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	// Now, set the SE_SYSTEMTIME_NAME privilege to our current process,
	// so we can call SetSystemTime().
	if (!OpenProcessToken(GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		PRINT("[CSP][CCTime] OpenProcessToken() failed with code %d",
			GetLastError());
		return;
	}
	if (!LookupPrivilegeValue(NULL, SE_SYSTEMTIME_NAME, &luid))
	{
		PRINT("[CSP][CCTime] LookupPrivilege() failed with code %d",
			GetLastError());
		CloseHandle(hToken);
		return;
	}

	ZeroMemory(&tp, sizeof(tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Adjust Token privileges.
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
			&oldtp, &dwSize))
	{
		PRINT("[CSP][CCTime] AdjustTokenPrivileges() failed with code %d",
			GetLastError());
		CloseHandle(hToken);
		return;
	}

	// Set time.
	if (!SetSystemTime(&systime))
	{
		PRINT("[CSP][CCTime] SetSystemTime() failed with code %d\n",
			GetLastError());
		CloseHandle(hToken);
		return;
	}

	// Disable SE_SYSTEMTIME_NAME again.
	AdjustTokenPrivileges(hToken, FALSE, &oldtp, dwSize, NULL, NULL);
	if (GetLastError() != ERROR_SUCCESS)
	{
		PRINT("[CSP][CCTime] AdjustTokenPrivileges() failed with code %d",
			GetLastError());
		CloseHandle(hToken);
		return;
	}

	CloseHandle(hToken);
#endif

#endif

#elif defined (_LINUX)

	struct timeval time;

	time.tv_sec = sec;
	time.tv_usec = 0;

	pthread_mutex_lock(&TimeMutex);
	settimeofday(&time, NULL);
	pthread_mutex_unlock(&TimeMutex);

#endif

	return;
}
