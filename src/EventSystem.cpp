
#include "EventSystem.h"
#include "EventSystem_defines.h"

#ifdef _WIN32
#include <windows.h>
#endif

EventSystem::EventSystem(void) :
m_uiFloodLimit(DEFAULT_EVENT_FLOOD_LIMIT)
{}

void EventSystem::Initialize(size_t uiFloodLimit)
{
	if (uiFloodLimit > 0)
		m_uiFloodLimit = uiFloodLimit;
}

void EventSystem::Shutdown(void)
{
	FlushEvents();
	m_CallbackMap.clear();
}

void EventSystem::QuerySubscribers(const Event& e)
{	
	std::multimap<Event::event_key, Event::event_delegate>::iterator itr = m_CallbackMap.begin();
	for (; itr != m_CallbackMap.end(); itr++)
	{
		if (!strcmp(e.GetEventID(),itr->first))
			itr->second(e);
	}
}

void EventSystem::RegisterCallback(Event::event_key evtKey, void (*fn)(const Event&))
{
	if(!IsRegistered(evtKey, fn))
	{
		Event::event_delegate evtDel;
		evtDel = fn;

		m_CallbackMap.insert(std::make_pair(evtKey, evtDel));
	}
}

bool EventSystem::IsRegistered(Event::event_key evtKey,void (*fn)(const Event&)) const
{
	Event::event_delegate evtDel;
	evtDel = fn;

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

void EventSystem::Unregister(Event::event_key evtKey, void (*fn)(const Event&))
{
	Event::event_delegate evtDel;
	evtDel = fn;

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

void EventSystem::UnregisterAll(void (*fn)(const Event&))
{
	Event::event_delegate evtDel;
	evtDel = fn;

	std::multimap<Event::event_key, Event::event_delegate>::iterator itr = m_CallbackMap.begin();

	while(itr != m_CallbackMap.end())
	{
		if((*itr).second == evtDel)
			itr = m_CallbackMap.erase(itr);
		else
			itr++;
	}
}

void EventSystem::sendEvent(Event::event_key evtKey, void* pSender, Event::event_priority_level ePriority)
{
	if(m_EventQueue.size() >= m_uiFloodLimit)
		return;

	Event* pEvent = m_EventFactory.Create();
	pEvent->m_EventID = evtKey;
	pEvent->m_pSender = pSender;
	pEvent->m_ePriority = ePriority;

	if(ePriority == Event::event_priority_immediate)
	{
		QuerySubscribers((*pEvent));
		m_EventFactory.Destroy(pEvent);
	}
	else
		m_EventQueue.push(pEvent);
}

void EventSystem::FlushEvents(void)
{
	while(!m_EventQueue.empty())
	{
		m_EventFactory.Destroy(m_EventQueue.top());
		m_EventQueue.pop();
	}
}

void EventSystem::ProcessEvents(void)
{
	while(!m_EventQueue.empty())
	{
		QuerySubscribers((*m_EventQueue.top()));
		m_EventFactory.Destroy(m_EventQueue.top());
		m_EventQueue.pop();
	}
}