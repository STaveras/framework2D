#pragma once

#include "Event.h"
#include "Factory.h"
#include <deque>
#include <list>
#include <map>
#include <queue>

class EventSystem
{
	size_t m_uiFloodLimit;

	Factory<Event> m_EventFactory;

	std::multimap<Event::event_key, Event::event_delegate> m_CallbackMap;
	std::priority_queue<Event*, std::deque<Event*>, Event::event_compare> m_EventQueue;

	void _querySubscribers(const Event& e);

public:
	EventSystem(void);
	~EventSystem(void){}

	size_t getFloodLimit(void) const { return m_uiFloodLimit; }
	void setFloodLimit(size_t uiFloodLimit) { m_uiFloodLimit = uiFloodLimit; }

	void initialize(size_t uiFloodLimit);
	void shutdown(void);

	template<class T>
	void registerCallback(Event::event_key evtKey, T* p, void (T::*fn)(const Event&));
	void registerCallback(Event::event_key evtKey, void (*fn)(const Event&));

	template<class T>
	bool isRegistered(Event::event_key evtKey, T* p, void (T::*fn)(const Event&)) const;
	bool isRegistered(Event::event_key evtKey, void (*fn)(const Event&)) const;

	template<class T>
	void unregister(Event::event_key evtKey, T* p, void (T::*fn)(const Event&));
	void unregister(Event::event_key evtKey, void (*fn)(const Event&));
	
	//template<class T>
	//void UnregisterAll(T* p); 
	// TODO: Unregister object from everything associated with it

	template<class T>
	void unregisterAll(T* p, void (T::*fn)(const Event&));
	void unregisterAll(void (*fn)(const Event&));

	template<class T>
	void sendEvent(const T& evt, void* pSender = NULL, Event::event_priority_level ePriority = Event::event_priority_normal);
	void sendEvent(Event::event_key evtKey, void* pSender = NULL, Event::event_priority_level ePriority = Event::event_priority_normal);
	
	typedef void (EventHandler)(const Event&);

	void flushEvents(void);
	void processEvents(void);
};

#include "EventSystem.hpp"