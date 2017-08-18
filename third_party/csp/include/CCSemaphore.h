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
 * \brief CCSemaphore class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCSemaphore class
class CCSemaphore
{
private:
	struct CTSemaphoreMember* m;

public:
	//! The constructor
	CCSemaphore(void) { m = NULL; }
	//! The destructor
	virtual ~CCSemaphore(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool FlagCreate(void) { return m != NULL; }

	//! Create the instance
	bool Create(long count);
	//! Destroy the instance
	virtual void Destroy(void);

	//! Decreases the counter of CCSemaphore.
	threadsafe void Take(void);
	//! Trys to decrease the counter of CCSemaphore.
	threadsafe bool Try(unsigned long msec = 0);
	//! Increases the counter of CCSemaphore.
	threadsafe void Give(void);
};




/*!
 * \class CCSemaphore
 *
 * CCSemaphore class supplies the function that creates and manages
 * semaphores.
 */




/*!
 * \fn bool CCSemaphore::Create(long count)
 *
 * This function creates a CCSemaphore object.
 *
 * \param[in] count (long)
 *        Semaphore counter
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCSemaphore::Destroy()
 *
 * \par Example:
 * \code
 *
 *     CCSemaphore semaphore;
 *     ...
 *     if (semaphore.Create() == true)
 *     {
 *         ...
 *         semaphore.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCSemaphore::FlagCreate(void)
 *
 * Check if the instance was created.
 * 
 * \return (bool)
 *        true on success, otherwise false.
 * 
 * \see CCSemaphore::Create()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn void CCSemaphore::Destroy()
 *
 * This function destroys the CCSemaphore object.
 * 
 * \see CCSemaphore::Create()
 */




/*!
 * \fn void CCSemaphore::Take(void)
 *
 * This function suspends the calling thread until the semaphore
 * value is non-zero. Then it decreases the counter of CCSemaphore.
 *
 * \see CCSemaphore::Give(), CCSemaphore::Try()
 *
 * \par Example:
 * \code
 *
 *     CCSemaphore semaphore;
 *     ...
 *     semaphore.Take();
 *     ...
 *     semaphore.Give();
 *     ...
 *
 * \endcode
 */




/*!
 * \fn bool CCSemaphore::Try(unsigned long msec)
 *
 * This function tries to decrease the counter of CCSemaphore.
 * If the counter is a non-zero value, the counter is decreased and
 * returned immediately. If the counter is zero, the function fails
 * and immediately returns without waiting.
 *
 * \param[in] msec (unsigned long)
 *        Time-out interval(millisecond)		
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCSemaphore::Give(), CCSemaphore::Take()
 *
 * \par Example:
 * \code
 *
 *     CCSemaphore semaphore;
 *     ...
 *     semaphore.Try(1000);
 *     ...
 *     semaphore.Give();
 *     ...
 *
 * \endcode
 */




/*!
 * \fn void CCSemaphore::Give(void)
 *
 * This function increases the counter of CCSemaphore.
 * If the semaphore count was 0, it is unlocked.
 *
 * \see CCSemaphore::Take(), CCSemaphore::Try()
 */




