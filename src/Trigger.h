// File: Trigger.h
#pragma once
#include <string>
struct Trigger
{
	enum TYPE
	{
		NONE,
		EVENT,
		ANIMATION,
		EFFECT,
		SOUND
	}Type;

	enum MODE
	{
		ONCE,
		ALWAYS
	}Mode;

	std::string Value;

private:
	bool _bTriggered;
	void _Trigger(void);
	Trigger(void);

public:
	explicit Trigger(TYPE type,std::string value,MODE mode = ONCE):Type(type),Value(value),Mode(mode),_bTriggered(false){}
	Trigger(const Trigger& t);
	~Trigger(void){}

	void LoadFromFile(const char* szFilename);

	void Activate(void);
	void reset(void);

	bool operator==(const Trigger& rhs) const { return (Type == rhs.Type && Mode == rhs.Mode && Value == rhs.Value); }
	void operator()(void) { Activate(); }
};
// Author: Stanley Taveras