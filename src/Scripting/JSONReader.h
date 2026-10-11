// JSONReader.h
#pragma once

#include <simdjson.h>

#include <cstdint>
#include <string>

// Typed reads for the engine's JSON data files (Tiled maps and tilesets, animations, ...).
// Each reader takes either a value looked up by key (element["key"], which carries an error
// when the key is missing) or an element already in hand, and returns fallback when the
// value is missing or of another type.
namespace Scripting {
namespace JSON {

	typedef simdjson::simdjson_result<simdjson::dom::element> Value;

	// Integers and decimals alike.
	float readFloat(simdjson::dom::element element, float fallback = 0.0f);
	// Rounded to the nearest integer.
	int readInt(simdjson::dom::element element, int fallback = 0);
	// Decimals are rounded to the nearest integer.
	int64_t readInt64(simdjson::dom::element element, int64_t fallback = 0);
	// true/false, or an integer that is true when non-zero.
	bool readBool(simdjson::dom::element element, bool fallback = false);
	std::string readString(simdjson::dom::element element, const std::string& fallback = "");
	// Any scalar as text: strings as they are, true/false, numbers in decimal. Empty for
	// anything else.
	std::string readText(simdjson::dom::element element);

	inline float readFloat(const Value& value, float fallback = 0.0f) { return value.error() ? fallback : readFloat(value.value_unsafe(), fallback); }
	inline int readInt(const Value& value, int fallback = 0) { return value.error() ? fallback : readInt(value.value_unsafe(), fallback); }
	inline int64_t readInt64(const Value& value, int64_t fallback = 0) { return value.error() ? fallback : readInt64(value.value_unsafe(), fallback); }
	inline bool readBool(const Value& value, bool fallback = false) { return value.error() ? fallback : readBool(value.value_unsafe(), fallback); }
	inline std::string readString(const Value& value, const std::string& fallback = "") { return value.error() ? fallback : readString(value.value_unsafe(), fallback); }
	inline std::string readText(const Value& value) { return value.error() ? std::string() : readText(value.value_unsafe()); }
}
}
