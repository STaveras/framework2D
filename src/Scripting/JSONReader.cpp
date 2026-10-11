// JSONReader.cpp

#include "JSONReader.h"

#include <cmath>
#include <sstream>

namespace Scripting {
namespace JSON {

	float readFloat(simdjson::dom::element element, float fallback)
	{
		if (element.is_null()) {
			return fallback;
		}
		if (element.is_double()) {
			return (float)element.get_double();
		}
		if (element.is_int64()) {
			return (float)element.get_int64();
		}
		if (element.is_uint64()) {
			return (float)element.get_uint64();
		}
		return fallback;
	}

	int readInt(simdjson::dom::element element, int fallback)
	{
		return (int)std::round(readFloat(element, (float)fallback));
	}

	int64_t readInt64(simdjson::dom::element element, int64_t fallback)
	{
		if (element.is_null()) {
			return fallback;
		}
		if (element.is_int64()) {
			return (int64_t)element.get_int64();
		}
		if (element.is_uint64()) {
			return (int64_t)element.get_uint64();
		}
		if (element.is_double()) {
			return (int64_t)std::llround((double)element.get_double());
		}
		return fallback;
	}

	bool readBool(simdjson::dom::element element, bool fallback)
	{
		if (element.is_null()) {
			return fallback;
		}
		if (element.is_bool()) {
			return (bool)element.get_bool();
		}
		if (element.is_int64()) {
			return element.get_int64() != 0;
		}
		if (element.is_uint64()) {
			return element.get_uint64() != 0;
		}
		return fallback;
	}

	std::string readString(simdjson::dom::element element, const std::string& fallback)
	{
		return element.is_string() ? std::string((std::string_view)element.get_string()) : fallback;
	}

	std::string readText(simdjson::dom::element element)
	{
		if (element.is_string()) {
			return std::string((std::string_view)element.get_string());
		}
		if (element.is_bool()) {
			return (bool)element.get_bool() ? "true" : "false";
		}
		if (element.is_int64()) {
			return std::to_string((int64_t)element.get_int64());
		}
		if (element.is_uint64()) {
			return std::to_string((uint64_t)element.get_uint64());
		}
		if (element.is_double()) {
			std::ostringstream stream;
			stream << (double)element.get_double();
			return stream.str();
		}
		return "";
	}
}
}
