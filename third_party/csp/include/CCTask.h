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
 * \brief CCTask class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCTask class
class CCTask: public CCHandler, public CCQueue, public CCThread
{
public:
	//! Event types
	enum CTEventType
	{
		EVENT_EXIT = 0,
		EVENT_PING = 1,
		EVENT_MAX  = 2
	};

private:
	struct CTTaskMember* m;

	//! Execute the main event loop
	static void m_ExecuteEventLoop(void* arg);

protected:
	//! Process Create events.
	virtual void t_Create(void);
	//! Process Destory events.
	virtual void t_Destroy(void);

	//! Process Idle events.
	virtual void t_Idle(void);

	//! Process events.
	bool t_OnEvent(const CTEvent * event);
	//! Process Exit events.
	virtual bool t_OnExit(const CTEvent * event);

public:
	//! The constructor
	CCTask(void) { m = NULL; }
	//! The destructor
	virtual ~CCTask(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool FlagCreate(void) { return m != NULL; }

	//! Create the instance
	bool Create(const char *name, unsigned long msec = 0, unsigned long stackSize = 0);
	//! Destroys the instance
	virtual void Destroy(void);

	//! Exit Task
	void Exit(void);

	//! Return the tasks name
	const char *Name(void);
	//! Return the CCThread ID of the task
	long Id(void);
	//! Set idle time
	void SetIdleTime(unsigned long msec);

	//! Execute the event loop for one iteration
	bool ExecuteEvent(void);

	//! Get current execution event
	void GetExecutionEvent(CTEvent* event);

	//! Send an event to the task
	static bool SendEvent(CTEvent* event, bool wait = false, bool priority = false);
};




/*!
 * \class CCTask
 *
 * This class, which inherits the CCThread, CCQueue, CCHandler classes,
 * creates a task and manages the task. Also, this class provides
 * the functions such as Create, Destroy, Id, etc.
 */




/*!
 * \fn bool CCTask::Create(const char* name, unsigned long msec, unsigned long stackSize)
 *
 * This function initializes the member variables and creates the CCHandler,
 * CCQueue, and CCThread.
 *
 * \param[in] name (const char*)
 *        The task name
 * \param[in] msec (unsigned long)
 *        The idle time
 * \param[in] stackSize (unsigned long)
 *        The stack size
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCTask::Destroy()
 * 
 * \par Example:
 * \code
 *
 *     CCTask task;
 *     ...
 *     if (task.Create("task") == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     task.Destroy();
 *
 * \endcode
 */




/*!
 * \fn void CCTask::Destroy(void)
 *
 * This function destroys the created CCTask object.
 *
 * \see CCTask::Create()
 */




/*!
 * \fn bool CCTask::FlagCreate(void)
 *
 * Check if the instance was created.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCTask::Create()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn const char* CCTask::Name(void)
 *
 * Get the task name
 *
 * \return (const char*)
 *        the task name
 * 
 * \par Example:
 * \code
 *
 *     CCTask task;
 *     ...
 *     if (task.Create("task") == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     char* name = task.Name();
 *     ...
 *     task.Destroy();
 *
 * \endcode
 */




/*!
 * \fn long CCTask::Id(void)
 *
 * Return the CCThread ID of the task
 *
 * \return (long)
 *        thread id
 *
 * \par Example:
 * \code
 *
 *     CCTask task;
 *     ...
 *     if (task.Create("task") == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     long id = task.Id();
 *     ...
 *     task.Destroy();
 *
 * \endcode
 */




/*!
 * \fn void CCTask::SetIdleTime(unsigned long msec)
 *
 * Set idle time
 *
 * \param[in] msec (unsigned long)
 *        the idle time to be set
 *
 * \par Example:
 * \code
 *
 *     CCTask task;
 *     ...
 *     if (task.Create("task") == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     task.SetIdleTime(1000);
 *     ...
 *     task.Destroy();
 *
 * \endcode
 */



/*!
 * \fn bool CCTask::ExecuteEvent(void)
 *
 * Execute the event loop for one iteration
 *
 * \return (bool)
 *        true on success, otherwise false
 *
 * \par Example:
 * \code
 *
 *     CCTask task;
 *     ...
 *     if (task.Create("task") == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     if (task.ExecuteEvent() == true)
 *     {
 *         ...
 *         task.GetExecutionEvent(&event);
 *         ...
 *     }
 *     ...
 *     task.Destroy();
 *
 * \endcode
 */




/*!
 * \fn void CCTask::GetExecutionEvent(CTEvent* event)
 *
 * Get current execution event
 *
 * \param[out] event (CTEvent*)
 *        Out buffer for the current event
 *
 * \see CCTask::ExecuteEvent()
 */




/*!
 * \fn bool CCTask::SendEvent(CTEvent* event, bool wait, bool priority)
 *
 * This function sends the event to the queue of the event receiver.
 * The receiver is determined by \c receiver field of \c event.
 *
 * \param[in] event (CTEvent*)
 *        The event information
 * \param[in] wait (bool)
 *        true if the event is to be sent synchronously
 * \param[in] priority (bool)
 *        true if the event must be processed with higher priority
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \par Example:
 * \code
 *
 *     CCTask task;
 *     CTEvent event;
 *     ...
 *     event.receiver = &task;
 *     event.type = EVENT_EXIT;
 *     if (CCTask::SendEvent(&event) == false)
 *     {
 *         return false;
 *     }
 *     ...
 *
 * \endcode
 */




