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
 * \brief CCQueue class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CTEvent structure
struct CTEvent
{
    class CCHandler* receiver; //!< The Pointer to the handler that receives the event
    unsigned long    type;     //!< The event type

    union CTEventParam
    {
        long  l[2];            //!< 2 \c long type parameters
        short s[4];            //!< 4 \c short type parameters
        char  c[8];            //!< 8 \c char type parameters
    } param;                   //!< Parameters of an event

    void* ret;                 //!< The return value
    void* reserved;            //!< Reserved Area, Internal Use Only
};




//! CTEventCallback type
typedef bool (*CTEventCallback)(const CTEvent* event, const void* param);




//! CCQueue class
class CCQueue
{
private:
	struct CTQueueMember* m;

public:
	//! The constructor
	CCQueue(void) { m = NULL; }
	//! The destructor
	virtual ~CCQueue(void) { ASSERT(m == NULL); }
	//! Check if the instance was created
	bool FlagCreate(void) { return m != NULL; }

	//! Create the instance
	bool Create(void);
	//! Destroy the instance
	virtual void Destroy(void);

	//! Returns the number of events in the Queue
	threadsafe unsigned long Count(void);
	//! Returns the number of events in the Queue with specified receiver
	threadsafe unsigned long Count(const CCHandler* receiver);
	//! Returns the number of events in the Queue with specified receiver and type
	threadsafe unsigned long Count(const CCHandler* receiver, unsigned long type);

	//! Put an event To the queue
	threadsafe bool Put(CTEvent* event, bool wait = false, bool priority = false);
	//! Get an event from the queue
	threadsafe bool Get(CTEvent* event, unsigned long msec = INFINITY);
	//! Peek an event from the queue
	threadsafe bool Peek(CTEvent* event, unsigned long msec = INFINITY);

	//! Remove the events
	threadsafe void Remove(unsigned long* count);
	//! Remove the events with the specified receiver.
	threadsafe void Remove(const CCHandler* receiver, unsigned long* count);
	//! Remove the events with the specified receiver and type.
	threadsafe void Remove(const CCHandler* receiver, unsigned long type, unsigned long* count);
};




/*!
 * \struct CTEvent
 *
 * The CTEvent structure contains event information.
 */




/*!
 * \class CCQueue
 *
 * The CCQueue class offers insertion, deletion, etc. of the events.
 */




/*!
 * \fn bool CCQueue::Create(void)
 *
 * Create the CCQueue
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         if( queue.FlagCreate() == true )
 *         {
 *              ...
 *              queue.Destroy();
 *         }
 *     }
 *
 * \endcode
 */




 /*!
 * \fn bool CCQueue::FlagCreate(void)
 *
 * Check if the instance was created.
 * 
 * \return (bool)
 *        true on success, otherwise false.
 * 
 * \see CCQueue::Create()
 *
 * \cond RA_CONFIGURATION
 * CODE_GEN_EXCLUDE
 * \endcond
 */




/*!
 * \fn void CCQueue::Destroy(void)
 *
 * Destroy the CCQueue
 *
 * \see CCQueue::Create()
 */




/*!
 * \fn unsigned long CCQueue::Count(void)
 *
 * This function retrieves the number of elements in queue.
 *
 * \return (unsigned long)
 *        the number of contents in queue
 *
 * \see CCQueue::Put()
 */




/*!
 * \fn unsigned long CCQueue::Count(const CCHandler* receiver)
 *
 * This function retrieves the number of elements in queue with the specified receiver.
 *
 * \param[in] receiver (const CCHandler*)
 *        Receiver of the events to find
 *
 * \return (unsigned long)
 *        the number of contents in queue with the specified receiver.
 *
 * \see CCQueue::Put()
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     long count;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         count = queue.Count(&handler);
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn unsigned long CCQueue::Count(const CCHandler* receiver, unsigned long type)
 *
 * This function retrieves the number of elements in queue with the specified receiver with type.
 *
 * \param[in] receiver (const CCHandler*)
 *        Receiver of the events to find.
 * \param[in] type (unsigned long)
 *        Type of events to find.
 *        Refer to \ref CTEvent "CTEvent types"
 *
 * \return (unsigned long)
 *        the number of contents in queue with the specified receiver with type.
 *
 * \see CCQueue::Put()
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     long count;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         count = queue.Count(&handler, EVENT_TEST1);
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCQueue::Put(CTEvent* event, bool wait, bool priority)
 *
 * This function adds the event into the queue.
 *
 * \param[in] event (CTEvent*) 
 *        Added event information
 * \param[in] wait (bool) 
 *        Waiting the inserting event is done
 * \param[in] priority (bool) 
 *        Event priority
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     CTEvent event;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         queue.Put(&event);
 *         unsigned long count = queue.Count();
 *         
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCQueue::Get(CTEvent* event, unsigned long msec)
 *
 * This function retrieves the event from the queue.
 *
 * \param[out] event (CTEvent*) 
 *        Retrieved event information
 * \param[in] msec (unsigned long)
 *        Timeout in milliseconds
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     CTEvent event;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         queue.Get(&event);
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn bool CCQueue::Peek(CTEvent* event, unsigned long msec)
 *
 * This function peeks the event from the queue.
 *
 * \param[out] event (CTEvent*) 
 *        Current event information
 * \param[in] msec (unsigned long)
 *        Timeout in milliseconds
 *
 * \return (bool)
 *        true on success, otherwise false.
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     CTEvent event;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         queue.Peek(&event);
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn void CCQueue::Remove(unsigned long* count)
 *
 * This function deletes the events in the queue.
 *
 * \param[out] count (unsigned long*) 
 *        Count of deleted events
 *
 * \see CCQueue::Count(void)
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     long count;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         queue.Remove(&count);
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn void CCQueue::Remove(const CCHandler* receiver, unsigned long* count)
 *
 * This function deletes the events with the specified receiver.
 *
 * \param[in] receiver (const CCHandler*)
 *        Receiver of the events to delete
 * \param[out] count (unsigned long*) 
 *        Count of deleted events
 *
 * \see CCQueue::Count(const CCHandler*)
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     long count;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         queue.Remove(&handler, &count);
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




/*!
 * \fn void CCQueue::Remove(const CCHandler* receiver, unsigned long type, unsigned long* count)
 *
 * This function deletes the events with the specified receiver and type.
 *
 * \param[in] receiver (const CCHandler*)
 *        Receiver of the events to delete.
 * \param[in] type (unsigned long)
 *        Type of the events to delete.
 *        Refer to \ref CTEvent "CTEvent types"
 * \param[out] count (unsigned long*) 
 *        Count of deleted events.
 *
 * \see CCQueue::Count(const CCHandler*, unsigned long)
 *
 * \par Example:
 * \code
 *
 *     CCQueue queue;
 *     long count;
 *     ...
 *     if (queue.Create() == true)
 *     {
 *         ...
 *         queue.Remove(&handler, EVENT_TEST1, &count);
 *         ...
 *         queue.Destroy();
 *     }
 *
 * \endcode
 */




