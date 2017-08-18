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
 * \brief CCMem class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCMem class
class CCMem
{
private:
	//! The private constructor that prevents instancing
	CCMem(void) {}

public:
	//! Sets the buffer to a specified value.
	static void Set(void* dest, int value, unsigned long size);
	//! Compares values in two buffers.
	static long Diff(const void* dest, const void* source, unsigned long size);
	//! Copies values from the source buffer to the destination buffer.
	static void Copy(void* dest, const void* source, unsigned long size);
	//! Moves one buffer to another.
	static void Move(void* dest, const void* source, unsigned long size);
};




/*!
 * \class CCMem
 *
 * CCMem class provides the functions used to manage memory. Functions used for 
 * memory allocation and deallocation are not provided. Thus, the user has to 
 * allocate and deallocate memory with new and delete operators.
 */




/*!
 * \fn void CCMem::Set(void* dest, int value, unsigned long size)
 *
 * This function sets buffers to a specified value.
 *
 * \param[out] dest (void*)
 *        Pointer to the destination buffer
 * \param[in] value (int)
 *        Value to set
 * \param[in] size (unsigned long)
 *        Number of value
 *
 * \par Example:
 * \code
 *
 *     char buffer[256];
 *     CCMem::Set(buffer, 0, sizeof(buffer));
 *
 * \endcode
 */




/*!
 * \fn long CCMem::Diff(const void* dest, const void* source, unsigned long count)
 *
 * The Diff() function compares the first \p count bytes of the memory areas \p 
 * dest and \p source. It returns an integer less than, equal to, or greater 
 * than zero if \p dest is found, to be less than, to match, or to be greater 
 * than \p source, respectively.
 *
 * \param[in] dest (const void*) 
 *        The destination buffer
 * \param[in] source (const void*) 
 *        The source buffer
 * \param[in] count (unsigned long)
 *        The number of bytes to compare
 *
 * \return (long)
 *        The Diff() function returns an integer less than, equal to, or 
 *        greater than zero if the first \p count bytes of \p dest is found, to be 
 *        less than, to match, or to be greater than the first \p count bytes of \p 
 *        source, respectively.
 *
 * \see CCString::Diff(), CCString::Length()
 *
 * \par Example:
 * \code
 *
 *     char dest[256] = "aaaa";
 *     char src[256] = "aaaa";
 *     if (CCMem::Diff(dest, src, CCString::Length(src)) == 0)
 *          ...
 *
 * \endcode
 */




/*!
 * \fn void CCMem::Copy(void* dest, const void* source, unsigned long count) 
 *
 * The Copy() function copies \p count bytes from memory area \p source to 
 * memory area \p dest. The memory areas may not overlap. Use Move() if the 
 * memory areas do overlap.
 *
 * \param[out] dest (void*) 
 *        The new buffer
 * \param[in] source (const void*)
 *        The buffer to copy from
 * \param[in] count (unsigned long)
 *        The number of bytes to copy
 *
 * \see CCMem::Move()
 *
 * \par Example:
 * \code
 *
 *     char dest[256];
 *     char src[256] = "aaaa";
 *     CCMem::Copy(dest, src, CCString::Length(src))
 *
 * \endcode
 */




/*!
 * \fn void CCMem::Move(void* dest, const void* source, unsigned long count) 
 *
 * The Move() function copies \p count bytes from memory area \p source to 
 * memory area \p dest. The memory areas may overlap.
 *
 * \param[out] dest (void*) 
 *        The new buffer
 * \param[in] source (const void*)
 *        The buffer to copy from
 * \param[in] count (unsigned long)
 *        The number of bytes to copy
 *
 * \see CCMem::Copy()
 *
 * \par Example:
 * \code
 *
 *     char dest[256];
 *     char src[256] = "aaaa";
 *     CCMem::Move(dest, src, CCString::Length(src))
 *
 * \endcode
 */
