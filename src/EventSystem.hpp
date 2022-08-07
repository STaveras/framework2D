
#pragma once

#include "EventSystem_defines.h"

template<class T>
void EventSystem::RegisterCallback(Event::event_key evtKey, T* p, void (T::*fn)(const Event&))
{
	if(!IsRegistered(evtKey, p, fn))
	{
		Event::event_delegate evtDel;
		evtDel.bind(p, fn);
		m_CallbackMap.insert(std::make_pair(evtKey, evtDel));
	}
}

template<class T>
bool EventSystem::IsRegistered(Event::event_key evtKey, T* p, void (T::*fn)(const Event&)) const
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
void EventSystem::Unregister(Event::event_key evtKey, T* p, void (T::*fn)(const Event&))
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
void EventSystem::UnregisterAll(T* p, void (T::*fn)(const Event&))
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

	Event* pEvent = m_EventFactory.CreateDerived<T>(e);
	pEvent->m_pSender = pSender;
	pEvent->m_ePriority = ePriority;

	if(pEvent->m_ePriority == Event::event_priority_immediate)
	{
		QuerySubscribers((*pEvent));
		m_EventFactory.Destroy(pEvent);
	}
	else
		m_EventQueue.push(pEvent);
}