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
 * \brief CCHandler class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCHandler class
class CCHandler
{
private:
	struct CTHandlerMember* m;

protected:
	//! The constructor
	CCHandler(void) { m = NULL; }
	//! The destructor
	virtual ~CCHandler(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool t_FlagCreate(void) { return m != NULL; }

	//! Create the instance
	bool t_Create(const class CCTask* task);
	//! Destroy the instance
	virtual void t_Destroy(void);

	//! Handle events, called from OnEvent()
	virtual bool t_OnEvent(const CTEvent* event) = 0;

public:
	//! Get the associated task
	CCTask* Task(void);
	//! The event handler
	void OnEvent(const CTEvent* event);
};




/*!
 * \class CCHandler
 *
 * This class provides a general event handler object. With this class, you can 
 * create an object that are signaled from a task when an event associated th 
 * the object is arrived.
 */




/*!
 * \fn bool CCHandler::t_FlagCreate(void) 
 *  
 * Check if the instance was created
 *
 * \return (bool)
 *        true on success, otherwise false.
 * 
 * \see CCHandler::t_Create(), CCHandler::t_Destroy()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn bool CCHandler::t_Create(const class CCTask* task)
 *
 * This function creates the CCHandler object.
 *
 * \param[in] task (const CCTask*)
 *        Pointer to the CCTask to set in CCHandler.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCHandler::t_Destroy()
 */




/*!
 * \fn void CCHandler::t_Destroy(void)
 *
 * This function destroys the specified CCHandler object.
 *
 * \see CCHandler::t_Create()
 */




/*!
 * \fn bool CCHandler::t_OnEvent(const CTEvent * event)
 *
 * When an event occurs, this function is called from OnEvent() function.
 *
 * \param[in] event (const CTEvent*)
 *        event Event information to process
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCHandler::OnEvent()
 */




/*!
 * \fn void CCHandler::OnEvent(const CTEvent* event)
 *
 * When the event happens, executes the virtual t_OnEvent() function.
 *
 * \param[in] event (const CTEvent*)
 *        Event information
 *
 * \see CCHandler::t_OnEvent()
 */




/*!
 * \fn CCTask* CCHandler::Task(void)
 *
 * This function returns the CCTask pointer in CCHandler.
 *
 * \return (CCTask*)
 *        pointer set in CCHandler
 */




