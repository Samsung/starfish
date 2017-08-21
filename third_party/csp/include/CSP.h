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
 * \brief The main header file of Core Software Platform
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




#ifdef NULL
#undef NULL
#endif

//! The NULL definition
/*!
 *
 * This macro can be used as some parameter values that accept the NULL.
 */
#define NULL (0)




#ifdef INFINITY
#undef INFINITY
#endif

//! The inifinite number
/*!
 *
 * This macro can be used as some parameter values that accept the infinite
 * number.
 */
#define INFINITY (~0)




#ifdef INVALID
#undef INVALID
#endif

//! The invalid number
/*!
 *
 * This macro can be used as some parameter that accept the invalid parameter.
 */
#define INVALID (-1)




// #define VERSION 20200




//! The type declaration to specify thread-safe
/*!
 *
 * This type declaration can be used to specify thread-safe.
 * Any member function follow after this type declaration means that
 * this member function is thread-safe member function.
 */
#define threadsafe




#if defined(_USE_ASSERT) || defined(_DOXYGEN)

// The target-specific implementation of ASSERT()
extern void _ASSERT(const char* expression, const char* file, unsigned long line);

//! Abort the program if assertion is false
/*!
 *
 * This macro aborts the entire program when the given expression is false,
 * displaying the information about the expression and the ASSERT() statement 
 * itself, including the source file name and the line number. This macro works 
 * only when _USE_ASSERT is defined. Otherwise, this macro is expanded to a 
 * null statement, resulting no overhead in program execution.
 *
 * \remark This macro is expanded to use _ASSERT() when _USE_ASSERT is defined.  
 * _ASSERT() must be implemented according to the target-specific environment.
 * 
 */
#define ASSERT(expression) if (!(expression)) _ASSERT(#expression, __FILE__, __LINE__);

#else

#define ASSERT(expression)

#endif




#if defined (_USE_PRINT) || defined(_DOXYGEN)

// The target-specific implementation of PRINT()
extern void _PRINT(const char* format = NULL, ...);

//! Print a formatted debug message.
/*!
 *
 * This macro prints a formatted debug message to the console (if exist). It 
 * only works when _USE_PRINT is defined, being expanded to _PRINT(), which 
 * requires target-specific implementation. If _USE_PRINT is not defined, the 
 * macro expands to a null statement.
 */
#define PRINT _PRINT

#else

#define PRINT if (false)

#endif




// OS Abstraction Layer & Utility Classes
#include "CCError.h"
#include "CCMem.h"
#include "CCString.h"
#include "CCMath.h"
#include "CCTime.h"
#include "CCSemaphore.h"
#include "CCMutex.h"
#include "CCList.h"
#include "CCQueue.h"
#include "CCDispatcher.h"
#include "CCThread.h"
#include "CCHandler.h"
#include "CCTimer.h"

// Application & Service Task Class
#include "CCTask.h"




//! The main function of a Shadow application
/*!
 *
 * This function is the main function of a Core Software Platform based application.
 * After initialization of Core Software Platform, this function is called,
 * and after this function completed, Core Software Platform is finalized
 * and the program terminates its execution.
 *
 * Every application developer must define this function in their
 * source code when linking with Core Software Platform library.
 */
// int Main(void);


// CSP Initialization
void init_csp();

// CSP Deinitialization
void deinit_csp();
