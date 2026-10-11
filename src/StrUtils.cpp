#include "StrUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace StrUtils
{

std::string trim(std::string_view value)
{
	size_t start = 0;
	size_t end = value.size();

	while (start < end && std::isspace((unsigned char)value[start])) {
		++start;
	}

	while (end > start && std::isspace((unsigned char)value[end - 1])) {
		--end;
	}

	return std::string(value.substr(start, end - start));
}

std::string toLower(std::string_view value)
{
	std::string lowered(value);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});
	return lowered;
}

bool iEquals(std::string_view left, std::string_view right)
{
	if (left.size() != right.size()) {
		return false;
	}

	for (size_t i = 0; i < left.size(); ++i) {
		if (std::tolower((unsigned char)left[i]) != std::tolower((unsigned char)right[i])) {
			return false;
		}
	}

	return true;
}

bool isTruthy(const char* value)
{
	if (!value) {
		return false;
	}
	return isTruthy(std::string_view(value));
}

bool isTruthy(std::string_view value)
{
	if (value.empty()) {
		return false;
	}

	const std::string lowered = toLower(trim(value));
	return lowered == "1" ||
		lowered == "true" ||
		lowered == "yes" ||
		lowered == "on";
}

std::vector<std::string> split(std::string_view input, char delimiter, bool trimTokens)
{
	std::vector<std::string> tokens;
	size_t start = 0;

	while (start <= input.size()) {
		size_t end = input.find(delimiter, start);
		if (end == std::string_view::npos) {
			end = input.size();
		}

		std::string token(input.substr(start, end - start));
		if (trimTokens) {
			token = trim(token);
		}
		tokens.push_back(std::move(token));

		if (end == input.size()) {
			break;
		}
		start = end + 1;
	}

	return tokens;
}

std::vector<std::string> splitAny(std::string_view input, std::string_view delimiters, bool trimTokens, bool skipEmpty)
{
	std::vector<std::string> tokens;
	if (delimiters.empty()) {
		std::string token(input);
		if (trimTokens) {
			token = trim(token);
		}
		if (!skipEmpty || !token.empty()) {
			tokens.push_back(std::move(token));
		}
		return tokens;
	}

	size_t start = 0;
	while (start <= input.size()) {
		size_t end = input.find_first_of(delimiters, start);
		if (end == std::string_view::npos) {
			end = input.size();
		}

		std::string token(input.substr(start, end - start));
		if (trimTokens) {
			token = trim(token);
		}

		if (!skipEmpty || !token.empty()) {
			tokens.push_back(std::move(token));
		}

		if (end == input.size()) {
			break;
		}
		start = end + 1;
	}

	return tokens;
}

std::string sanitizeIdentifier(std::string_view value, char replacement, const char* fallback)
{
	if (value.empty()) {
		return std::string(fallback ? fallback : "");
	}

	std::string sanitized;
	sanitized.reserve(value.size());
	for (char c : value) {
		if (std::isalnum((unsigned char)c) || c == '_' || c == '-') {
			sanitized.push_back(c);
		}
		else {
			sanitized.push_back(replacement);
		}
	}

	return sanitized.empty() ? std::string(fallback ? fallback : "") : sanitized;
}

float floatFromString(const std::string& text, float fallback)
{
	char* end = nullptr;
	const float value = std::strtof(text.c_str(), &end);
	return (end == text.c_str()) ? fallback : value;
}

int intFromString(const std::string& text, int fallback)
{
	char* end = nullptr;
	const long value = std::strtol(text.c_str(), &end, 10);
	return (end == text.c_str()) ? fallback : (int)value;
}

vector2 vectorFromString(const std::string& text, vector2 fallback)
{
	vector2 output(0.0f, 0.0f);
	return (std::sscanf(text.c_str(), "%f , %f", &output.x, &output.y) == 2) ? output : fallback;
}

std::string vectorToString(vector2 v)
{
	std::ostringstream output;
	output << v.x << "," << v.y;
	return output.str();
}

RECT rectFromString(const std::string& text, const RECT& fallback)
{
	if (text.empty()) {
		return fallback;
	}

	RECT output{};
	std::string input(text);
	for (char& c : input) {
		if (c == '{' || c == '}') {
			c = ' ';
		}
	}

	int width = -1;
	int height = -1;
	int numericValues[4]{ 0, 0, 0, 0 };
	int numericCount = 0;
	bool usedKeyValues = false;

	size_t start = 0;
	while (start < input.size()) {
		size_t end = input.find(',', start);
		if (end == std::string::npos) {
			end = input.size();
		}

		std::string token = trim(std::string_view(input).substr(start, end - start));
		if (!token.empty()) {
			size_t eq = token.find('=');
			if (eq != std::string::npos) {
				usedKeyValues = true;
				std::string key = trim(std::string_view(token).substr(0, eq));
				std::string value = trim(std::string_view(token).substr(eq + 1));
				int parsedValue = std::atoi(value.c_str());

				if (key == "X") {
					output.left = parsedValue;
				}
				else if (key == "Y") {
					output.top = parsedValue;
				}
				else if (key == "Width") {
					width = parsedValue;
				}
				else if (key == "Height") {
					height = parsedValue;
				}
			}
			else if (numericCount < 4) {
				numericValues[numericCount++] = std::atoi(token.c_str());
			}
		}

		start = end + 1;
	}

	if (!usedKeyValues && numericCount >= 4) {
		output.left = numericValues[0];
		output.top = numericValues[1];
		width = numericValues[2];
		height = numericValues[3];
	}

	if (width >= 0) {
		output.right = output.left + width;
	}
	if (height >= 0) {
		output.bottom = output.top + height;
	}

	return output;
}

std::string rectToString(const RECT& rect)
{
	return std::to_string(rect.left) + "," +
		std::to_string(rect.top) + "," +
		std::to_string(rect.right - rect.left) + "," +
		std::to_string(rect.bottom - rect.top);
}

} // namespace StrUtils
