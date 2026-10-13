// File: Trigger.h
#pragma once
#include "Maths.h"

#include <cstdint>
#include <string>
#include <vector>

class Tile;

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
	// Painted tiles that make up the trigger (non-owning), e.g. a row of hazard tiles. When
	// set, their tileset collision shapes (or cells, if they have none) are its volume.
	std::vector<Tile*> tiles;
};

enum class TraversalTriggerType
{
	Unknown,
	Goal,
	Destination,
	Checkpoint,
	Killzone,
	TimeBonus,
	StaminaPickup,
	Hazard
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
	// Hazards re-apply damage every interval while the character overlaps them.
	float interval = 0.0f;
	float cooldownRemaining = 0.0f;
	// Hazards throw the character this way on each hit (pixels/second, +y down).
	vector2 push = vector2(0.0f, 0.0f);
	float pushLockSeconds = 0.0f;
	// When set, the horizontal push points away from the hazard, toward whichever side the
	// character touched it from (e.g. a hanging thorny vine).
	bool pushAway = false;
	// Tiles whose shapes are the trigger's volume; position and size then only bound them.
	std::vector<Tile*> tiles;
	// Destinations carry the relative path of the map to load once reached.
	std::string nextMap;
	// Set on a destination the character starts a run touching, so an entry spawn beside a
	// section boundary doesn't leave straight away; cleared once the character steps off.
	bool waitForExit = false;
};
// Author: Stanley Taveras
