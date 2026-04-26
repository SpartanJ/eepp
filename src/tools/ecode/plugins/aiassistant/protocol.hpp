#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace ecode {

struct LLMCacheConfiguration {
	int maxCacheAnchors;
	int minTotalToken;
	bool shouldSpeculate;
};

struct LLMModel {
	std::size_t hash{ 0 };
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

struct ACPAgent {
	bool enabled{ true };
	std::string name;
	std::string command;
	std::vector<std::string> args;
	std::unordered_map<std::string, std::string> environment;
};

using ACPAgents = std::map<std::string, ACPAgent>;

} // namespace ecode