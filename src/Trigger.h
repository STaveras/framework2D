// File: Trigger.h
#pragma once
#include "Maths.h"

#include <cstdint>
#include <string>
#include <vector>
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
	explicit Trigger(TYPE type,std::string value,MODE mode = ONCE):Type(type),Mode(mode),Value(value),_bTriggered(false){}
	Trigger(const Trigger& t);
	~Trigger(void){}

	void LoadFromFile(const char* szFilename);

	void Activate(void);
	void reset(void);

	bool operator==(const Trigger& rhs) const { return (Type == rhs.Type && Mode == rhs.Mode && Value == rhs.Value); }
	void operator()(void) { Activate(); }
};

struct TriggerPropertyDescriptor
{
	std::string name;
	std::string type;
	std::string value;
};

struct LevelTriggerDescriptor
{
	int layerId = -1;
	std::string layerName;
	int objectId = -1;
	std::string name;
	std::string typeName;
	int64_t gid = 0;
	vector2 position;
	vector2 size;
	float rotation = 0.0f;
	bool visible = true;
	bool isPoint = false;
	bool isEllipse = false;
	bool hasPolygon = false;
	bool hasPolyline = false;
	std::vector<vector2> polygonPoints;
	std::vector<vector2> polylinePoints;
	std::vector<TriggerPropertyDescriptor> properties;
};

enum class TraversalTriggerType
{
	Unknown,
	Goal,
	Checkpoint,
	Killzone,
	TimeBonus,
	StaminaPickup
};

struct TraversalTrigger
{
	TraversalTriggerType type = TraversalTriggerType::Unknown;
	std::string name;
	vector2 position = vector2(0.0f, 0.0f);
	vector2 size = vector2(0.0f, 0.0f);
	float value = 0.0f;
	bool oneShot = true;
	bool consumed = false;
};
// Author: Stanley Taveras
