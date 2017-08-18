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
 * \brief CCTime class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCTime class
class CCTime
{
public:
	//! CTTime structure
	struct CTData
	{
		unsigned short year;   //!< Year (AD)
		unsigned char  month;  //!< Month (1-12)
		unsigned char  day;    //!< Day (1-31)
		unsigned char  hour;   //!< Hour (0-23)
		unsigned char  minute; //!< Minute (0-59)
		unsigned char  second; //!< Second (0-59)
	};

private:
	//!< The private constructor that prevents instancing
	CCTime(void) {}

public:
	//! Delay for a specified amount of time
	static void Sleep(unsigned long msec);
	//! Get tick count
	static unsigned long Tick(void);

	//! Get time since the Epoch
	static unsigned long EpochTime(void);
	//! Set time since the Epoch
	static void SetEpochTime(unsigned long sec);

	//! Convert time since the Epoch to local time
	static void ConvertEpochToLocalTime(CTData* datetime, unsigned long sec, long timezone, bool daylight);
	//! Convert local time to time since the Epoch
	static void ConvertLocalToEpochTime(unsigned long* sec, const CTData* datetime, long timezone, bool daylight);
};




/*!
 * \class CCTime
 *
 * This class provides several static member functions for getting and setting
 * current time.
 */




/*!
 * \fn void CCTime::Sleep(unsigned long msec)
 *
 * This function delays the execution for the specified time.
 *
 * \param[in] msec (unsigned long)
 *        The amount of time to sleep in milliseconds.
 *
 * \par Example:
 * \code
 *
 *     ...
 *     CCTime::Sleep(1000);
 *     ...
 *
 * \endcode
 */




/*!
 * \fn unsigned long CCTime::Tick(void)
 *
 * This function retrieves the number of milliseconds that have elapsed since
 * some epoch time, such as when the system was started.
 *
 * The elapsed time is returned as an unsigned long value.
 * Therefore, the time will wrap around to zero if the system is run
 * continuously for 49.7 days.
 *
 * \return (unsigned long)
 *        the number of milliseconds elapsed since the epoch time.
 *
 * \par Example:
 * \code
 *
 *     ...
 *     unsigned long start = CCTime::Tick();
 *     ...
 *
 * \endcode
 */




/*!
 * \fn unsigned long CCTime::EpochTime(void)
 *
 * This function returns the time since Epoch (00:00:00 UTC, January 1, 1970)
 * measured in seconds.
 * 
 * \return (unsigned long)
 *        the time since Epoch measured in seconds.	
 *
 * \par Example:
 * \code
 *
 *     ...
 *     unsigned long epoch = CCTime::EpochTime();
 *     ...
 *
 * \endcode
 */




/*!
 * \fn void CCTime::SetEpochTime(unsigned long sec)
 *
 * This function sets the time since Epoch (00:00:00 UTC, January 1, 1970) 
 * measured in seconds.
 *
 * \param[in] sec (unsigned long)
 *        the time since Epoch measured in seconds
 *
 * \par Example:
 * \code
 *
 *     unsigned long epoch;
 *     ...
 *     CCTime::SetEpochTime(epoch);
 *     ...
 *
 * \endcode
 */




/*!
 * \fn void CCTime::ConvertEpochToLocalTime(CTData* localtime, unsigned long sec, long timezone, bool daylight)
 *
 * This function takes seconds elapsed since the Epoch (00:00:00 UTC, January 
 * 1, 1970) and converts it to a local time stored in a CTData structure.
 *
 * \param[out] localtime (CTData*)
 *        a pointer to a CTData structure where the converted local time is stored
 * \param[in] sec (unsigned long)
 *        the time since the Epoch
 * \param[in] timezone (long)
 *        timezone value (the minutes east of Greenwich)
 * \param[in] daylight (bool)
 *        daylight saving time flag (true if daylight saving time is in force)
 *
 * \par Example:
 * \code
 *
 *     CCTime:CTData time;
 *     unsigned long epoch = CCTime::EpochTime();
 *     CCTime::ConvertEpochToLocalTime(&time, epoch, 540, false);
 *
 * \endcode
 */




/*!
 * \fn void CCTime::ConvertLocalToEpochTime(unsigned long* sec, const CTData* localtime, long timezone, bool daylight)
 *
 * This function converts local time to time since the Epoch
 * (00:00:00 UTC, January 1, 1970).
 *
 * \param[out] sec (unsigned long*)
 *        the time since the Epoch
 * \param[in] localtime (const CTData*)
 *        a pointer to a CTData structure where the local time is stored
 * \param[in] timezone (long)
 *        timezone value (the minutes east of Greenwich)
 * \param[in] daylight (bool)
 *        daylight saving time flag (true if daylight saving time is in force)
 *
 * \par Example:
 * \code
 *
 *     CCTime:CTData time;
 *     unsigned long epoch;
 *     ...
 *     CCTime::ConvertLocalToEpochTime(&epoch, &time, 540, false);
 *
 * \endcode
 */
