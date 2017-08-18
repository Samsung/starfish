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
 * \brief CCTimer class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCTimer class
class CCTimer : public CCDispatcher
{
public:
	//! condition type to subscribe
	enum CTCondType
	{
		COND_TIMER_REPORT = 0  //!< condition to report timer tick
	};

	enum CTTimerState
	{
		TIMER_STATE_STOP  = 0, //!< Timer stopped
		TIMER_STATE_START = 1  //!< Timer started
	};

private:
	struct CTTimerMember* m;

public:
	//! A Constructor
	CCTimer(void) { m = NULL; }
	//! A Destructor
	virtual ~CCTimer(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool FlagCreate(void) { return m != NULL; }

	//! Create the instance
	bool Create(void);
	//! Destroy the instance
	virtual void Destroy(void);

	//! Subscribes an event to occur when the specified situation occurs.
	bool SubscribeEvent(int condType, const CTEvent* event, bool priority = false);
	//! Unsubscribes an event to occur when the specified situation occurs.
	bool UnsubscribeEvent(int condType, const CTEvent* event = NULL); 

	//! Start the timer with given parameters
	void Start(unsigned long interval, unsigned long count = INFINITY);
	//! Stop the timer
	void Stop(void);

	//! Returns the timer state.
	int State(void);

};




/*!
 * \class CCTimer
 *
 * This class provides the functions to handle timer events
 */




/*!
 * \fn bool CCTimer::Create(void)
 *
 * Create the timer instance
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCTimer::Destroy()
 * 
 * \par Example:
 * \code
 *
 *     CCTimer timer;
 *     ...
 *     if (timer.FlagCreate() == false )
 *     {
 *         if (timer.Create() == false)
 *         {
 *             return false;
 *         }
 *      }
 *     ...
 *     task.Destroy();
 *
 * \endcode
 */




/*!
 * \fn bool CCTimer::FlagCreate(void)
 *
 * Check if the instance was created.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see CCTimer::Create()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn void CCTimer::Destroy(void)
 *
 * Destroy the instance
 *
 * \see CCTimer::Create()
 */




/*!
 * \fn bool CCTimer::SubscribeEvent(int condType, const CTEvent* event, bool priority)
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
 * \see CCTimer::UnsubscribeEvent()
 *
 * \par Example:
 * \code
 *
 *     const int MY_EVENT_TIMER_REPORT = 1111;
 *     ...
 *     CCTimer timer;
 *     ...
 *     if (timer.Create(200) == false)
 *     {
 *         return false;
 *     }
 * 
 *     event.receiver = &task;
 *     event.type = MY_EVENT_TIMER_REPORT;
 *     if (timer.SubscribeEvent(CCTimer::COND_TIMER_REPORT, &event) == false)
 *     {
 *         timer.Destroy();
 *         return false;
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCTimer::UnsubscribeEvent(int condType, const CTEvent* event)
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
 * \see CCTimer::SubscribeEvent()
 *
 * \par Example:
 * \code
 *
 *     ...
 *     if (timer.UnsubscribeEvent(CCTimer::COND_TIMER_REPORT) == false)
 *     {
 *         return false;
 *     }
 *     ...
 *
 * \endcode
 */




/*!
 * \fn void CCTimer::Start(unsigned long interval, unsigned long count)
 *
 * This function start the timer with the interval and count of the specified values.
 *
 * \param[in] interval (unsigned long)
 *        New time-out value
 * \param[in] count (unsigned long)
 *        New iteration value
 *
 * \see CCTimer::Stop()
 *
 * \par Example:
 * \code
 *
 *     CCTimer timer;
 *     timer.Create();
 *     ...
 *     timer.Start(3000, 100);     // 3000ms,  100 times
 *
 * \endcode
 */




/*!
 * \fn void CCTimer::Stop(void)
 *
 * Stop Timer
 *
 * \see CCTimer::Start()
 */




/*!
 * \fn int CCTimer::State(void)
 *
 * Returns the timer State
 * 
 * \return (int)
 *        Timer state #CTTimerState
 *
 * \see CCTimer::Start(), CCTimer::Stop()
 */




