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
 * \brief CCString class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCString class
class CCString
{
private:
	//! The private constructor that prevents instancing
	CCString(void) {}

public:
	//! Appends a string to the destination.
	static void Concate(char* dest, const char* source);
	//! Appends a string to the destination.
	static void Concate(char* dest, const char* source, unsigned long size);
	//! Compares strings.
	static long Diff(const char* dest, const char* source);
	//! Compares strings.
	static long Diff(const char* dest, const char* source, unsigned long size);
	//! Copies characters of one string to another.
	static void Copy(char* dest, const char* source);
	//! Copies characters of one string to another.
	static void Copy(char* dest, const char* source, unsigned long size);
	//! Gets the length of a string.
	static unsigned long Length(const char* str);
	//! Locates the specified character in a string.
	static char* Char(const char* str, char ch);
	//! Writes formatted output using a pointer to a string.
	static void Print(char* str, const char* format, ...);
	//! Converts strings to integer.
	static int Integer(const char* str);
};




/*!
 * \class CCString
 *
 * CCString class provides the functions used to manage strings.
 */




/*!
 * \fn void CCString::Concate(char* dest, const char* source)
 *
 * This function appends the \p source string to the \p dest string overwriting 
 * the `\\0' character at the end of \p dest, and then adds a terminating `\\0' 
 * character at the end. The strings may not overlap, and the \p dest string 
 * must have enough space for the result.
 *
 * \param[out] dest (char*) 
 *        The null-terminated destination string
 * \param[in] source (const char*)
 *        The null-terminated source string
 *
 * \see CCString::Concate(char*, const char*, unsigned long) 
 *
 * \par Example:
 * \code
 *
 *     char dest[256] = "123";
 *     char src[256] = "456";
 *     CCString::Concate(dest, src);
 *
 * \endcode
 */




/*!
 * \fn void CCString::Concate(char* dest, const char* source, unsigned long size)
 *
 * This function is similar to the Concate(char*, const char*) function, except 
 * that it will use at most \p size characters from \p source. Since the result 
 * is always terminated with `\\0', at most \p size + 1 characters are written.
 *
 * \param[out] dest (char*) 
 *        The null-terminated destination string
 * \param[in] source (const char*)
 *        The null-terminated source string
 * \param[in] size (unsigned long)
 *        The number of characters to append
 *
 * \see CCString::Concate(char*, const char*)
 *
 * \par Example:
 * \code
 *
 *     char dest[256] = "123";
 *     char src[256] = "456";
 *     CCString::Concate(dest, src, CCString::Length(src));
 *
 * \endcode
 */




/*!
 * \fn long CCString::Diff(const char* dest, const char* source)
 *
 * This function compares the two strings \p dest and \p source. It returns an 
 * integer less than, equal to, or greater than zero if \p dest is found, to be 
 * less than, to match, or to be greater than \p source, respectively.
 *
 * \param[in] dest (const char*)
 *        The first null-terminated string to compare
 * \param[in] source (const char*)
 *        The second null-terminated string to compare
 *
 * \return (long)
 *        The function returns an integer less than, equal to, or greater than 
 *        zero if \p dest is found to be less than, to match, or to be greater than \p 
 *        source, respectively.
 *
 * \see CCString::Diff(const char*, const char*, unsigned long) 
 *
 * \par Example:
 * \code
 *
 *     char dest[256] = "123";
 *     char src[256] = "456";
 *     if (CCString::Diff(dest, src) == 0)
 *         ...
 *
 * \endcode
 */




/*!
 * \fn long CCString::Diff(const char* dest, const char* source, unsigned long size)
 *
 * This function is similar to Diff(const char*, const char*), except it only 
 * compares the first (at most) \p size characters of \p dest and \p source.
 *
 * \param[in] dest (const char*)
 *        The first null-terminated string to compare
 * \param[in] source (const char*)
 *        The second null-terminated string to compare
 * \param[in] size (unsigned long)
 *        The number of characters to compare
 *
 * \return (long)
 *        The function returns an integer less than, equal to, or greater than 
 *        zero if \p dest (or the first \p size bytes thereof) is found, to be less 
 *        than, to match, or to be greater than \p source, respectively.
 *
 * \see CCString::Diff(const char*, const char*) 
 *
 * \par Example:
 * \code
 *
 *     char dest[256] = "123";
 *     char src[256] = "456";
 *     if (CCString::Diff(dest, src, CCString::Length(src)) == 0)
 *         ...
 *
 * \endcode
 */




/*!
 * \fn void CCString::Copy(char* dest, const char* source)
 *
 * This function copies the string pointed to by \p source (including the 
 * terminating `\\0' character) to the array pointed to by \p dest. The strings 
 * may not overlap, and the destination string \p dest must be large enough to 
 * receive the copy.
 *
 * \param[out] dest (char*)
 *        The destination string
 * \param[in] source (const char*)
 *        The source string
 *
 * \see CCString::Copy(char*, const char*, unsigned long), CCMem::Copy(), CCWString::Copy()
 *
 * \par Example:
 * \code
 *
 *     char dest[256];
 *     char src[256] = "456";
 *     CCString::Copy(dest, src);
 *
 * \endcode
 */




/*!
 * \fn void CCString::Copy(char* dest, const char* source, unsigned long size)
 *
 * This function is similar to Copy(char*. const char*), except that not more 
 * than \p size bytes of \p source are copied. Thus, if there is no null byte 
 * among the first \p size bytes of \p source, the result will not be 
 * null-terminated.
 *
 * In the case where the length of \p source is less than that of \p size, the 
 * remainder of \p dest will be padded with nulls.
 *
 * \param[out] dest (char*) 
 *        The destination string
 * \param[in] source (const char*)
 *        The source string
 * \param[in] size (unsigned long)
 *        The number of characters to copy
 *
 * \see CCString::Copy(char*, const char*), CCMem::Copy(), CCWString::Copy()
 *
 * \par Example:
 * \code
 *
 *     char dest[256];
 *     char src[256] = "456";
 *     CCString::Copy(dest, src, CCString::Length(src));
 *
 * \endcode
 */




/*!
 * \fn unsigned long CCString::Length(const char* str)
 *
 * This function calculates the length of the string \p str, not including the 
 * terminating `\\0' character.
 *
 * \param[in] str (const char*)
 *        The null-terminated string
 *
 * \return (unsigned long)
 *        The function returns the number of characters in \p str. No return 
 *        value is reserved to indicate an error.
 *
 * \see CCString::Copy(), CCString::Diff()
 */




/*!
 * \fn char* CCString::Char(const char* str, char ch)
 *
 * This function returns a pointer to the first occurrence of the character \p 
 * ch in the string \p str.
 *
 * \param[in] str (const char*)
 *        The string to be searched
 * \param[in] ch (char)
 *        The character to locate
 *
 * \return (char*)
 *        The function returns a pointer to the matched character or NULL if 
 *        the character is not found.
 *
 * \par Example:
 * \code
 *
 *     char buffer[256] = "123456";
 *     char* pos = CCString::Char(buffer, '2');
 *
 * \endcode
 */




/*!
 * \fn void CCString::Print(char* str, const char* format, ...)
 *
 * This function writes formatted output using a pointer to a string.
 *
 * \param[out] str (char*)
 *        Storage location for output
 * \param[in] format (const char*)
 *        Format specification
 * \param[in]  (...)
 *        Variable arguments
 *
 * \par Example:
 * \code
 *
 *     char dest[256];
 *     char src[256] = "456";
 *     CCString::Copy(dest, src, CCString::Length(src));
 *
 * \endcode
 */




/*!
 * \fn int CCString::Integer(const char* str)
 *
 * The function converts the initial portion of the string \p str to integer. 
 *
 * \param[in] str (const char*)
 *        The string to be converted
 *
 * \return (int)
 *        The converted value.
 *
 * \par Example:
 * \code
 *
 *     char buffer[256] = "123";
 *     int value = CCString::Integer(buffer);
 *
 * \endcode
 */




