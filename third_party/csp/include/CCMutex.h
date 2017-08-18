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
 * \brief CCMutex class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCMutex class
class CCMutex
{
private:
	struct CTMutexMember* m;

public:
	//! The constructor
	CCMutex(void) { m = NULL; }
	//! The destructor
	virtual ~CCMutex(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool FlagCreate(void) { return m != NULL; }

	//! Create the instance
	bool Create(void);
	//! Destroy the instance
	virtual void Destroy(void);

	//! Locks the created CCMutex object.
	threadsafe void Lock(void);
	//! Unlocks the CCMutext object.
	threadsafe void Unlock(void);
};




/*!
 * \class CCMutex
 *
 * CCMutex class provides functions used to create and manage mutexes.
 */




/*!
 * \fn bool CCMutex::Create(void)
 *
 * This function creates a CCMutex object.
 *
 * \return (boo)
 *        true on success, otherwise false.
 *
 * \see CCMutex::Destroy()
 *
 * \par Example:
 * \code
 *
 *     CCMutex mutex;
 *     if (mutex.Create() == false)
 *     {
 *         return false;
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCMutex::FlagCreate(void)
 *
 * Check if the instance was created.
 * 
 * \return (bool)
 *        true on success, otherwise false.
 * 
 * \see CCMutex::Create()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn void CCMutex::Destroy(void)
 *
 * This function destroys the CCMutex object.
 *
 * \see CCMutex::Create()
 *
 * \par Example:
 * \code
 *
 *     CCMutex mutex;
 *     if (mutex.Create() == true)
 *     {
 *         ...
 *         mutex.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn void CCMutex::Lock(void)
 *
 * This function locks the created CCMutex object.
 *
 * \see CCMutex::Unlock()
 *
 * \par Example:
 * \code
 *
 *     CCMutex mutex;
 *     ...
 *     mutex.Lock();
 *     ...
 *     mutex.Unlock();
 *
 * \endcode
 */




/*!
 * \fn void CCMutex::Unlock(void)
 *
 * This function unlocks the CCMutex object.
 *
 * \see CCMutex::Lock()
 */
