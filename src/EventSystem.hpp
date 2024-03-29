
#pragma once

#include "EventSystem_defines.h"

template<class T>
void EventSystem::registerCallback(Event::event_key evtKey, T* p, void (T::*fn)(const Event&))
{
	if(!isRegistered(evtKey, p, fn))
	{
		Event::event_delegate evtDel;
		evtDel.bind(p, fn);
		m_CallbackMap.insert(std::make_pair(evtKey, evtDel));
	}
}

template<class T>
bool EventSystem::isRegistered(Event::event_key evtKey, T* p, void (T::*fn)(const Event&)) const
{
	Event::event_delegate evtDel;
	evtDel.bind(p, fn);

	std::pair<std::multimap<Event::event_key, Event::event_delegate>::const_iterator,
		std::multimap<Event::event_key, Event::event_delegate>::const_iterator> range = m_CallbackMap.equal_range(evtKey);

	std::multimap<Event::event_key, Event::event_delegate>::const_iterator itr = range.first;

	for(; itr != range.second; itr++)
	{
		if((*itr).second == evtDel)
			return true;
	}

	return false;
}

template<class T>
void EventSystem::unregister(Event::event_key evtKey, T* p, void (T::*fn)(const Event&))
{
	Event::event_delegate evtDel;
	evtDel.bind(p, fn);

	std::pair<std::multimap<Event::event_key, Event::event_delegate>::iterator,
		std::multimap<Event::event_key, Event::event_delegate>::iterator> range = m_CallbackMap.equal_range(evtKey);

	std::multimap<Event::event_key, Event::event_delegate>::iterator itr = range.first;

	for(; itr != range.second; itr++)
	{
		if((*itr).second == evtDel)
		{
			m_CallbackMap.erase(itr);
			break;
		}
	}
}

template<class T>
void EventSystem::unregisterAll(T* p, void (T::*fn)(const Event&))
{
	Event::event_delegate evtDel;
	evtDel.bind(p, fn);

	std::multimap<Event::event_key, Event::event_delegate>::iterator itr = m_CallbackMap.begin();

	while(itr != m_CallbackMap.end())
	{
		if((*itr).second == evtDel)
			itr = m_CallbackMap.erase(itr);
		else
			itr++;
	}
}

//template<class T>
//void EventSystem::UnregisterAll(T* p)
//{
//	std::multimap<Event::event_key, Event::event_delegate>::iterator itr = m_CallbackDatabase.begin();
//
//	for(;itr != m_CallbackDatabase.end();itr++)
//	{
//		if((*itr).second.GetMemento().m_pthis == p)
//			itr = m_CallbackDatabase.erase(itr);
//	}
//}

template<class T>
void EventSystem::sendEvent(const T& e, void* pSender, Event::event_priority_level ePriority)
{
	if(m_EventQueue.size() >= m_uiFloodLimit)
		return;

	Event* pEvent = m_EventFactory.createDerived<T>(e);
	pEvent->_sender = pSender;
	pEvent->_priorityLevel = ePriority;

	if(pEvent->_priorityLevel == Event::event_priority_immediate)
	{
		_querySubscribers((*pEvent));
		m_EventFactory.destroy(pEvent);
	}
	else
		m_EventQueue.push(pEvent);
}