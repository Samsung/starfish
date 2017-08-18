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
 * \brief CCDispatcher class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCDispatcher class
class CCDispatcher
{
protected:
	//! A Constructor
	CCDispatcher(void) {}
	//! A Destructor
	virtual ~CCDispatcher(void) {}

public:
	//! Subscribes an event to occur when the specified situation occurs
	virtual bool SubscribeEvent(int condType, const CTEvent* event, bool priority = false) = 0;
	//! Unsubscribes an event to occur when the specified situation occurs
	virtual bool UnsubscribeEvent(int condType, const CTEvent* event = NULL) = 0;
};




/*!
 * \class CCDispatcher
 *
 * Hierarchically, CCDispatcher is virtual class of active component. <br>
 * Active components are usually generate events of given condition. <br>
 */




/*!
 * \fn bool CCDispatcher::SubscribeEvent(int condType, const CTEvent* event, bool priority = false)
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
 *        The condition type to monitor. 
 * \param[in] event (const CTEvent*)
 *        An event to be sent when the condition is satisfied.
 * \param[in] priority (bool)
 *        Indicates whether the event must be sent with high priority or not.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see UnsubscribeEvent()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn bool CCDispatcher::UnsubscribeEvent(int condType, const CTEvent* event)
 *
 * Unsubscribes an event to occur when the specified situation occurs.
 * This function unsubscribes an event to be delivered whenever a specified condition
 * is satisfied.
 *
 * \param[in] condType (int)
 *        The condition type to monitor. CTCondType
 * \param[in] event (const CTEvent*)
 *        The event to be removed.
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \see SubscribeEvent()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




