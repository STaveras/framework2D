// File: Event.h
// Author: Stanley Taveras

#pragma once

#include <delegate/FastDelegate.h>
#include <functional>

// NOTE: For the 'sender' object, we really should throw around structs with additional event information (like windows messages)

class Event
{
   friend class EventSystem;

public:

   // This is kinda dumb???
   enum event_priority_level
   {
      event_priority_immediate,
      event_priority_high,
      event_priority_above_normal,
      event_priority_normal,
      event_priority_below_normal,
      event_priority_low
   };

   typedef const char* event_key;
   typedef fastdelegate::FastDelegate1<const Event&> event_delegate;
   typedef struct _compare /*: public std::binary_function<Event, Event, bool>*/
   {
      inline bool operator()(Event* lhs, Event* rhs) const
      {
         return ((int)lhs->_priorityLevel > (int)rhs->_priorityLevel);
      }

      inline bool operator()(const Event& lhs, const Event& rhs) const
      {
         return ((int)lhs._priorityLevel > (int)rhs._priorityLevel);
      }
   } event_compare;

protected:
   event_priority_level	_priorityLevel;
   event_key            _eventKey;
   void*                _sender;
   // void*                m_pData;

public:
   Event(void) :_priorityLevel(event_priority_normal), _eventKey(""), _sender(NULL) {}
   Event(event_key eventKey, void* sender, event_priority_level priorityLevel = event_priority_normal) :_priorityLevel(priorityLevel), _eventKey(eventKey), _sender(sender) {}
   virtual ~Event(void) {}

   event_key getEventID(void) const { return _eventKey; }
   void*     getSender(void) const { return _sender; }

   bool operator==(const Event& rhs) const { return (!strcmp(_eventKey, rhs._eventKey) && _priorityLevel == rhs._priorityLevel && _sender == rhs._sender); }
   bool operator==(const event_key e) { return (bool)(!strcmp(_eventKey, e)); }
};