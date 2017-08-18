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
 * \brief CCThread class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCThread class
class CCThread : public CCDispatcher
{
public:
	//! Priority types
	enum CTPriorityType
	{
		PRIORITY_LOW    =   0, //!< Low priority
		PRIORITY_NORMAL =  50, //!< Normal priority
		PRIORITY_HIGH   = 100  //!< High priority
	};

    //! condition type to subscribe
	enum CTCondType
	{
		COND_THREAD_EXIT_REPORT = 0  //!< condition to report thread is exited
	};

private:
	struct CTThreadMember* m;

public:
	//! The constructor
	CCThread(void) { m = NULL; }
	//! The destructor
	virtual ~CCThread(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool FlagCreate(void) { return m != NULL; }

	//! Create the instance
	bool Create(void (*threadMain)(void*), void* arg, unsigned long stackSize = 0);
	//! Destroy the instance
	virtual void Destroy(void);

	//! Subscribes an event to occur when the specified situation occurs
	bool SubscribeEvent(int condType, const CTEvent* event, bool priority = false);
	//! Unsubscribes an event to occur when the specified situation occurs
	bool UnsubscribeEvent(int condType, const CTEvent* event = NULL);

	//! Detach Thread
	bool Detach(void);

	//! Returns the ID of the CCThread.
	long Id(void);
	//! Set the priority value for the thread
	bool SetPriority(int priority);

	//! The ID of the current executing thread
	static long SelfId(void);
	//! Set the priority value for the current executing thread.
	static bool SetSelfPriority(int priority);
};




/*!
 * \class CCThread
 *
 * This class provides functions used to create and manage threads.
 */




/*!
 * \fn bool CCThread::Create(void (*threadMain)(void*), void* arg, unsigned long stackSize)
 *
 * This function creates CCThread object.
 *
 * \param[in] threadMain (void (*threadMain)(void*))
 *        The pointer to the first function called after a new thread is spawn.
 * \param[in] arg (void*)
 *        A general argument passed to the start routine
 * \param[in] stackSize (unsigned long)
 *        Size of stack
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCThread::Destroy()
 *
 * \par Example:
 * \code
 *
 *     void ThreadMain(void* arg)
 *     {
 *         ...
 *     }
 *     ...
 *     CCThread thread;
 *     ...
 *     if (thread.FlagCreate() == false)
 *     {
 *         if (thread.Create(ThreadMain) == false)
 *         {
 *             return false;
 *         }
 *     }
 *
 *     if (thread.FlagCreate() == true)
 *          thread.Destroy();
 *
 * \endcode
 */




/*!
 * \fn bool CCThread::FlagCreate(void)
 *
 * Check if the instance was created.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCThread::Create()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn void CCThread::Destroy(void)
 *
 * This function destroys the CCThread.
 *
 * \see CCThread::Create()
 */




/*!
 * \fn bool CCThread::SubscribeEvent(int condType, const CTEvent* event, bool priority)
 * 
 * Subscribes an event to occur when the specified situation occurs.
 * 
 * A thread monitoring events of the specified type is created internally. 
 * (This thread usually inherits CCTask.) 
 * When this thread detects an event of the specified type, it creates a corresponding event structure
 * (SubscribeEvent() calls the event setting function of this thread) 
 * and sends the event to the event handler assigned in event.receiver.
 * 
 * SendEvent(CTEvent* event, bool wait, bool priority) of CCTask is used in order to accomplish these operations,
 * and the created event structure, wait and priority parameters received are used as the parameters.
 * 
 * \param[in] condType (int)
 *        The condition type to monitor. #CTCondType
 * \param[in] event (const CTEvent*)
 *        An event to be sent when the condition is satisfied.
 * \param[in] priority (bool)
 *        Indicates whether the event must be sent with high priority or not.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCThread::UnsubscribeEvent()
 *
 * \par Example:
 * \code
 *
 *     const int MY_EVENT_THREAD_TERMINATE = 1111;
 *     ...
 *     CCThread thread;
 *     ...
 *     if (thread.Create(ThreadMain) == false)
 *     {
 *         return false;
 *     }
 * 
 *     event.receiver = &task;
 *     event.type = MY_EVENT_THREAD_TERMINATE;
 *     if (thread.SubscribeEvent(CCThread::COND_THREAD_EXIT_REPORT, &event) == false)
 *     {
 *         thread.Destroy();
 *         return false;
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCThread::UnsubscribeEvent(int condType, const CTEvent* event)
 *
 * Unsubscribes an event to occur when the specified situation occurs.
 * This function unsubscribes an event to be delivered whenever a specified condition
 * is satisfied.
 *
 * \param[in] condType (int)
 *        The condition type to monitor. #CTCondType
 * \param[in] event (const CTEvent*)
 *        The event to be removed.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCThread::SubscribeEvent()
 *
 * \par Example:
 * \code
 *
 *     ...
 *     if (thread.UnsubscribeEvent(CCThread::COND_THREAD_EXIT_REPORT) == false)
 *     {
 *         return false;
 *     }
 *     ...
 *
 * \endcode
 */




/*!
 * \fn bool CCThread::Detach(void)
 *
 * Detach thread
 *
 * \return (bool)
 *        true on success, otherwise false
 *
 * \par Example:
 * \code
 *
 *     CCThread thread;
 *     ...
 *     if (thread.Create(ThreadMain) == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     thread.Detach();
 *
 * \endcode
 */




/*!
 * \fn long CCThread::Id(void)
 *
 * This function returns the ID of the thread.
 *
 * \return (long)
 *        ID of the created thread
 *
 * \see CCThread::SelfId()
 *
 * \par Example:
 * \code
 *
 *     CCThread thread;
 *     ...
 *     if (thread.Create(ThreadMain) == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     thread.Id();
 *
 * \endcode
 */




/*!
 * \fn bool CCThread::SetPriority(int priority)
 *
 * This function sets the priority of the thread.
 *
 * \param[in] priority (int)
 *        The priority value for the thread. This parameter can be one of the 
 *        CTPriorityType values: CCThread::PRIORITY_HIGH, 
 *        CCThread::PRIORITY_NORMAL, CCThread::PRIORITY_LOW.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCThread::SetSelfPriority()
 *
 * \par Example:
 * \code
 *
 *     CCThread thread;
 *     ...
 *     if (thread.Create(ThreadMain) == false)
 *     {
 *         return false;
 *     }
 *     ...
 *     thread.SetPriority(CCThread::PRIORITY_HIGH);
 *
 * \endcode
 */




/*!
 * \fn long CCThread::SelfId(void)
 *
 * This function returns the ID of the currently executing thread.
 *
 * \return (long)
 *        ID of the currently executing thread
 *
 * \see CCThread::Id()
 *
 * \par Example:
 * \code
 *
 *     ...
 *     long id = CCThread::SelfId();
 *     ...
 *
 * \endcode
 */




/*!
 * \fn bool CCThread::SetSelfPriority(int priority)
 *
 * This function sets the priority of the currently executing thread.
 *
 * \param[in] priority (int)
 *        The priority value for the thread. This parameter can be one of the 
 *        CTPriorityType values: CCThread::PRIORITY_HIGH, 
 *        CCThread::PRIORITY_NORMAL, CCThread::PRIORITY_LOW.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCThread::SetPriority()
 *
 * \par Example:
 * \code
 *
 *     ...
 *     bool ret = CCThread::SetSelfPriority(CCThread::PRIORITY_HIGH);
 *     ...
 *
 * \endcode
 */




