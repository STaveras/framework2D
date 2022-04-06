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

	void QuerySubscribers(const Event& e);

public:
	EventSystem(void);
	~EventSystem(void){}

	size_t GetFloodLimit(void) const { return m_uiFloodLimit; }
	void SetFloodLimit(size_t uiFloodLimit) { m_uiFloodLimit = uiFloodLimit; }

	void Initialize(size_t uiFloodLimit);
	void Shutdown(void);

	template<class T>
	void RegisterCallback(Event::event_key evtKey, T* p, void (T::*fn)(const Event&));
	void RegisterCallback(Event::event_key evtKey, void (*fn)(const Event&));

	template<class T>
	bool IsRegistered(Event::event_key evtKey, T* p, void (T::*fn)(const Event&)) const;
	bool IsRegistered(Event::event_key evtKey, void (*fn)(const Event&)) const;

	template<class T>
	void Unregister(Event::event_key evtKey, T* p, void (T::*fn)(const Event&));
	void Unregister(Event::event_key evtKey, void (*fn)(const Event&));
	
	//template<class T>
	//void UnregisterAll(T* p); 
	// TODO: Unregister object from everything associated with it

	template<class T>
	void UnregisterAll(T* p, void (T::*fn)(const Event&));
	void UnregisterAll(void (*fn)(const Event&));

	template<class T>
	void sendEvent(const T& evt, void* pSender = NULL, Event::event_priority_level ePriority = Event::event_priority_normal);
	void sendEvent(Event::event_key evtKey, void* pSender = NULL, Event::event_priority_level ePriority = Event::event_priority_normal);
	
	typedef void (EventHandler)(const Event&);

	void FlushEvents(void);
	void ProcessEvents(void);
};

#include "EventSystem.hpp"