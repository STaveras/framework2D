
#include "EventSystem.h"

#ifdef _WIN32
#include <windows.h>
#endif

EventSystem::EventSystem(void) :
m_uiFloodLimit(DEFAULT_EVENT_FLOOD_LIMIT)
{}

void EventSystem::initialize(size_t uiFloodLimit)
{
	if (uiFloodLimit > 0)
		m_uiFloodLimit = uiFloodLimit;
}

void EventSystem::shutdown(void)
{
	flushEvents();
	m_CallbackMap.clear();
}

void EventSystem::_querySubscribers(const Event& e)
{	
	std::multimap<Event::event_key, Event::event_delegate>::iterator itr = m_CallbackMap.begin();
	for (; itr != m_CallbackMap.end(); itr++)
	{
		if (!strcmp(e.getEventID(),itr->first))
			itr->second(e);
	}
}

void EventSystem::registerCallback(Event::event_key evtKey, void (*fn)(const Event&))
{
	if (!isRegistered(evtKey, fn)) {
		m_CallbackMap.insert(std::make_pair(evtKey, fn));
	}
}

bool EventSystem::isRegistered(Event::event_key evtKey,void (*fn)(const Event&)) const
{
	Event::event_delegate evtDel = fn;

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

void EventSystem::unregister(Event::event_key evtKey, void (*fn)(const Event&))
{
	Event::event_delegate evtDel = fn;

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

void EventSystem::unregisterAll(void (*fn)(const Event&))
{
	Event::event_delegate evtDel = fn;

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
	pEvent->_eventKey = evtKey;
	pEvent->_sender = pSender;
	pEvent->_priorityLevel = ePriority;

	if(ePriority == Event::event_priority_immediate)
	{
		_querySubscribers((*pEvent));
		m_EventFactory.Destroy(pEvent);
	}
	else
		m_EventQueue.push(pEvent);
}

void EventSystem::flushEvents(void)
{
	while(!m_EventQueue.empty())
	{
		m_EventFactory.Destroy(m_EventQueue.top());
		m_EventQueue.pop();
	}
}

void EventSystem::processEvents(void)
{
	while(!m_EventQueue.empty())
	{
		_querySubscribers((*m_EventQueue.top()));
		m_EventFactory.Destroy(m_EventQueue.top());
		m_EventQueue.pop();
	}
}