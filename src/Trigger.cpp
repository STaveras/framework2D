// File: Trigger.cpp
#include "Trigger.h"
#include "Engine2D.h"
#include "SDSParser.h"
#include "StrUtil.h"

Trigger::Trigger(void):
	Type(NONE),
	Mode(ONCE),
	_bTriggered(false)
{}

Trigger::Trigger(const Trigger& t):
	_bTriggered(false)
{
	Type = t.Type;
	Mode = t.Mode;
	Value = t.Value;
}

void Trigger::_Trigger(void)
{
	if (_bTriggered)
		return;

	switch(Type)
	{
	case EVENT:
		Engine2D::GetInstance()->GetEventSystem()->SendEvent(Value.c_str(),this);
		break;
	case ANIMATION:
		break;
	case EFFECT:
		break;
	case SOUND:
		break;
	}

	_bTriggered = true;
}

void Trigger::Activate()
{
	switch(Mode)
	{
	case ALWAYS:
		_Trigger();
		Reset();
		break;
	case ONCE:
		_Trigger();
		break;
	}
}

void Trigger::Reset()
{
	_bTriggered = false;
}

void Trigger::LoadFromFile(const char* szFilename)
{
	SDSParser parser;

	if(!parser.open(szFilename))
	{
		if(parser.setScope("Trigger"))
		{
			SDSNodeData* pData = NULL;

			pData = parser.getProperty("Type");

			if(pData)
			{
				if(streqls(pData->GetValue(), "Effect"))
					Type = EFFECT;
				else if(streqls(pData->GetValue(), "Event") || pData->GetValueAsInt() == 1)
					Type = EVENT;
				else if(streqls(pData->GetValue(), "Sound") || pData->GetValueAsInt() == 2)
					Type = SOUND;
				else
					Type = NONE;
			}

			pData = parser.getProperty("Mode");

			if(pData)
			{
				if(streqls(pData->GetValue(), "Always"))
					Mode = ALWAYS;
				else if(streqls(pData->GetValue(), "Once") || pData->GetValueAsInt() == 1)
					Mode = ONCE;
			}

			pData = parser.getProperty("Value");

			if(pData)
			{
				Value = pData->GetValue();
			}
		}

		parser.close();
	}
}
// Author: Stanley Taveras