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
   typedef struct _compare : public std::binary_function<Event, Event, bool>
   {
      inline bool operator()(Event* lhs, Event* rhs) const
      {
         return ((int)lhs->m_ePriority > (int)rhs->m_ePriority);
      }

      inline bool operator()(const Event& lhs, const Event& rhs) const
      {
         return ((int)lhs.m_ePriority > (int)rhs.m_ePriority);
      }
   }event_compare;

protected:
   event_priority_level	m_ePriority;
   event_key            m_EventID;
   void*                m_pSender;

public:
   Event(void) :m_ePriority(event_priority_normal), m_EventID(""), m_pSender(NULL) {}
   Event(event_key eventID, void* sender, event_priority_level ePriority = event_priority_normal) :m_ePriority(ePriority), m_EventID(eventID), m_pSender(sender) {}
   virtual ~Event(void) {}

   event_key	GetEventID(void) const { return m_EventID; }
   void*      GetSender(void) const { return m_pSender; }

   bool operator==(const Event& rhs) const { return (!strcmp(m_EventID, rhs.m_EventID) && m_ePriority == rhs.m_ePriority && m_pSender == rhs.m_pSender); }
   bool operator==(const event_key e) { return (bool)(!strcmp(m_EventID, e)); }
};