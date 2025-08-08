#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ecode {

struct LLMCacheConfiguration {
	int maxCacheAnchors;
	int minTotalToken;
	bool shouldSpeculate;
};

struct LLMModel {
	std::string name;
	std::string provider;
	std::optional<std::string> displayName;
	std::optional<std::size_t> maxTokens;
	std::optional<std::size_t> maxOutputTokens;
	std::optional<double> defaultTemperature;
	std::optional<LLMCacheConfiguration> cacheConfiguration;
	bool isEphemeral{ false };
	bool cheapest{ false };
	bool reasoning{ false };
	bool toolCalling{ false };
};

struct LLMProvider {
	bool enabled{ true };
	bool openApi{ false };
	std::string name;
	std::optional<std::string> displayName;
	std::string apiUrl;
	std::optional<std::string> fetchModelsUrl;
	std::optional<int> version;
	std::vector<LLMModel> models;
};

using LLMProviders = std::map<std::string, LLMProvider>;

} // namespace ecode
