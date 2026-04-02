#include "StrUtils.h"

#include <algorithm>
#include <cctype>

namespace StrUtils
{

std::string Trim(std::string_view value)
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

std::string ToLower(std::string_view value)
{
	std::string lowered(value);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});
	return lowered;
}

bool IEquals(std::string_view left, std::string_view right)
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

bool IsTruthy(const char* value)
{
	if (!value) {
		return false;
	}
	return IsTruthy(std::string_view(value));
}

bool IsTruthy(std::string_view value)
{
	if (value.empty()) {
		return false;
	}

	const std::string lowered = ToLower(Trim(value));
	return lowered == "1" ||
		lowered == "true" ||
		lowered == "yes" ||
		lowered == "on";
}

std::vector<std::string> Split(std::string_view input, char delimiter, bool trimTokens)
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
			token = Trim(token);
		}
		tokens.push_back(std::move(token));

		if (end == input.size()) {
			break;
		}
		start = end + 1;
	}

	return tokens;
}

std::vector<std::string> SplitAny(std::string_view input, std::string_view delimiters, bool trimTokens, bool skipEmpty)
{
	std::vector<std::string> tokens;
	if (delimiters.empty()) {
		std::string token(input);
		if (trimTokens) {
			token = Trim(token);
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
			token = Trim(token);
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

std::string SanitizeIdentifier(std::string_view value, char replacement, const char* fallback)
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

} // namespace StrUtils
