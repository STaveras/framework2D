
#include "InputEvent.h"

InputEvent::InputEvent(Event::event_key evtKey, void* pSender, VirtualGamePad* pGamePad, float fTime, ButtonID btnID):
Event(evtKey, pSender, Event::event_priority_high),
m_fTime(fTime),
m_pGamePad(pGamePad),
m_ButtonID(btnID)
{}