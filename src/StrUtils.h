#pragma once

#include "Types.h"

#include <string>
#include <string_view>
#include <vector>

namespace StrUtils
{
std::string trim(std::string_view value);
std::string toLower(std::string_view value);
bool iEquals(std::string_view left, std::string_view right);
bool isTruthy(const char* value);
bool isTruthy(std::string_view value);
std::vector<std::string> split(std::string_view input, char delimiter, bool trimTokens = true);
std::vector<std::string> splitAny(std::string_view input, std::string_view delimiters, bool trimTokens = true, bool skipEmpty = true);
std::string sanitizeIdentifier(std::string_view value, char replacement = '_', const char* fallback = "unnamed");

// Values written as text in data files (rects, offsets, numeric properties). Each reader
// returns fallback when the text doesn't hold a value.

// A leading decimal number ("1.5", " 2", "3px" reads 3).
float floatFromString(const std::string& text, float fallback);
// A leading base-10 integer.
int intFromString(const std::string& text, int fallback);

// "x,y"
vector2 vectorFromString(const std::string& text, vector2 fallback = vector2(0.0f, 0.0f));
std::string vectorToString(vector2 v);

// "x,y,width,height", or keyed: "X=0, Y=0, Width=64, Height=80" (braces allowed).
// fallback is used for empty text.
RECT rectFromString(const std::string& text, const RECT& fallback = RECT{});
// "x,y,width,height"
std::string rectToString(const RECT& rect);
} // namespace StrUtils
