#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace StrUtils
{
std::string Trim(std::string_view value);
std::string ToLower(std::string_view value);
bool IEquals(std::string_view left, std::string_view right);
bool IsTruthy(const char* value);
bool IsTruthy(std::string_view value);
std::vector<std::string> Split(std::string_view input, char delimiter, bool trimTokens = true);
std::vector<std::string> SplitAny(std::string_view input, std::string_view delimiters, bool trimTokens = true, bool skipEmpty = true);
std::string SanitizeIdentifier(std::string_view value, char replacement = '_', const char* fallback = "unnamed");
} // namespace StrUtils
