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
 * \brief CCList class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCList class
class CCList 
{
public:
	//! Add mode
	enum CTAddMode
	{
		ADD_BEFORE = 0, //!< The node is added behind the current position
		ADD_AFTER  = 1  //!< The node is added in front of the current position
	};

	//! List positions
	enum CTPositionType
	{
		POS_BEGIN   = 0, //!< Beginning of the list
		POS_CURRENT = 1, //!< Current position of the list
		POS_END     = 2  //!< End of the list
	};

private:
	struct CTListMember* m;

public:
	//! The constructor
	CCList(void) { m = NULL; }
	//! The destructor
	virtual ~CCList(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool FlagCreate(void) { return m != NULL; }

	//! Initialize the list.
	bool Create(void);
	//! Destroy the list and deletes all nodes.
	virtual void Destroy(void);

	//! Get the size of data.
	unsigned long Count(void);
	//! Get the data at the current list position. 
	void* Data(void);

	//! Adds data to the list.
	bool Add(const void* data, int mode = ADD_AFTER);
	//! Set current position of the list.
	bool SetPosition(int whence, int index);
	//! Remove an element from the list.
	void Remove(void);
};




/*!
 * \class CCList
 *
 * CCList class provides functions that create and manage linked lists.
 */




/*!
 * \fn bool CCList::Create(void)
 *
 * Creates a list and sets the member variables.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCList::Destroy()
 *
 * \par Example:
 * \code
 *
 *     CCList list;
 *     if (list.Create() == false)
 *     {
 *         return false;
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCList::FlagCreate(void)
 *
 * Check if the instance was created.
 * 
 * \return (bool)
 *        true on success, otherwise false.
 * 
 * \see CCList::Create()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn void CCList::Destroy(void)
 *
 * Destroys the mutex and deletes all nodes.
 *
 * \see CCList::Create()
 *
 * \par Example:
 * \code
 *
 *     CCList list;
 *     if (list.Create() == true)
 *     {
 *         ...
 *         list.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn unsigned long CCList::Count(void)
 *
 * This function returns the size of the list.
 *
 * \return (unsigned long)
 *        The size of the list.
 *
 * \see CCList::Add()
 *
 * \par Example:
 * \code
 *
 *     int count;
 *     void* data;
 *     CCList list;
 *     if (list.Create() == true)
 *     {
 *         ...
 *         list.Add(data, CCList::ADD_AFTER);
 *         count = list.Count();
 *         ...
 *         list.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn void* CCList::Data(void)
 *
 * This function returns the data at the current position in the list.
 *
 * \return (void*) 
 *        The data at the current position in the list.
 *
 * \see CCList::SetPosition(), CCList::Count()
 *
 * \par Example:
 * \code
 *
 *     void* data;
 *     int count;
 *     CCList list;
 *     if (list.Create() == true)
 *     {
 *         ...
 *         void* getData = list.Data();
 *         ...
 *         list.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCList::Add(const void* data, int mode)
 *
 * This function inserts \p data to the current position in the list.
 *
 * \param[in] data (const void*)
 *        The data to be inserted into the list
 * \param[in] mode (int)
 *        The position at which the data will be inserted (ADD_BEFORE, ADD_AFTER)
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \par Example:
 * \code
 *
 *     int count;
 *     void* data;
 *     CCList list;
 *     if (list.Create() == true)
 *     {
 *         ...
 *         list.Add(data, CCList::ADD_AFTER);
 *         count = list.Count();
 *         ...
 *         list.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCList::SetPosition(int whence, int index)
 *
 * This function sets the current position indicator. The new position is
 * determined by \p offset and \p whence. \p whence is the origin from which
 * the current position moves and can be \c POS_BEGIN, \c POS_CURRENT or \c POS_END.
 *
 * \param[in] whence (int)
 *        The origin
 * \param[in] index (int)
 *        The amount to move from the origin
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCList::Data();
 *
 * \par Example:
 * \code
 *
 *     void* data;
 *     int count;
 *     CCList list;
 *     ...
 *     if (list.SetPosition(CCList::POS_BEGIN, 0)
 *     {
 *         do
 *         {
 *             ...
 *         } while (list.SetPosition(CCList::POS_CURRENT, 1));
 *
 *     }
 * \endcode
 */




/*!
 * \fn void CCList::Remove(void)
 *
 * This function removes the element at the current position.
 *
 * \par Example:
 * \code
 *
 *     int count;
 *     void* data;
 *     CCList list;
 *     if (list.Create() == true)
 *     {
 *         ...
 *         list.Add(data, CCList::ADD_AFTER);
 *         ...
 *         list.Remove();
 *         ...
 *         list.Destroy();
 *     }
 *
 * \endcode
 */





