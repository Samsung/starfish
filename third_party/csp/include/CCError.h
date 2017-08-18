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
 * \brief CCError class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCError class
class CCError
{
private:
	//! The private constructor that prevents instancing
	CCError(void) {}

public:
	enum CTErrorType
	{
		ERROR_NONE,                //!< No Error
		ERROR_INVALID,             //!< Invalid argument
		ERROR_INITIALIZED_BEFORE,  //!< Already initialized
		ERROR_UNINITIALIZED,       //!< Not initialized
		ERROR_OVERFLOW,            //!< Overflow
		ERROR_UNDERFLOW,           //!< Stack underflow
		ERROR_SHORTAGE,            //!< Out of resource
		ERROR_NO_MEMORY,           //!< Out of memory
		ERROR_NO_SPACE,            //!< Out of space
		ERROR_NO_ENTRY,            //!< No entry found
		ERROR_UNSUPPORTED,         //!< Not supported (Invalid Operation)
		ERROR_INTERRUPTED,         //!< Interrupted
		ERROR_FAULT,               //!< Bad Address
		ERROR_AGAIN,               //!< Try again
		ERROR_ACCESS,              //!< Access denied
		ERROR_RESET,               //!< Reset
		ERROR_BLOCK,               //!< Operation would block
		ERROR_EXIST,               //!< Already exists
		ERROR_EXIST_INVALID,       //!< Already exists with invalid object
		ERROR_IN_USE,              //!< Already in use
		ERROR_IN_PROGRESS,         //!< Operation now in progress
		ERROR_TIME_OUT,            //!< Operation time out
		ERROR_NO_RESPONSE,         //!< Operation no response
		ERROR_UNKNOWN              //!< Unknown Error
	};

	//! Sets an error number.
	static void Set(int number);
	//! Gets an error number.
	static int Num(void);
};




/*!
 * \class CCError
 *
 * CCError class manages the error numbers. This class is used to check the 
 * type of error that occurred when a function failed.
 */




/*!
 * \fn void CCError::Set(int number)
 *
 * This function sets an error number, which can be retrieved by Num() 
 * function.
 *
 * \param[in] number (int)
 *        The error number
 *
 * \see CCError::Num()
 *
 * \par Example:
 * \code
 *
 *     CCError::Set(CCError::ERROR_INVALID);
 *
 * \endcode
 */




/*!
 * \fn int CCError::Num(void)
 *
 * This function returns the last error number.
 *
 * \return (int)
 *        The error number
 *
 * \see CCError::Set()
 *
 * \par Example:
 * \code
 *
 *     int num;
 *     num = CCError::Num();
 *
 * \endcode
 */





