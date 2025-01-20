#pragma once

#include <eepp/core/containers.hpp>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace std::literals;

using json = nlohmann::json;

using namespace EE;

namespace ecode {

static const auto REQUEST = "request"sv;
static const auto REDIRECT_STDERR = "redirectStderr"sv;
static const auto REDIRECT_STDOUT = "redirectStdout"sv;

struct Command {
	std::string command;
	std::vector<std::string> arguments;
	std::unordered_map<std::string, std::string> environment;
	bool isValid() const;
};

struct Connection {
	int port;
	std::string host;
	bool isValid() const;
};

struct BusSettings {
	std::optional<Command> command;
	std::optional<Connection> connection;

	bool isValid() const;
	bool hasCommand() const;
	bool hasConnection() const;
};

struct ProtocolSettings {
	bool linesStartAt1{ true };
	bool columnsStartAt1{ true };
	bool pathFormatURI{ false };
	bool redirectStderr{ false };
	bool redirectStdout{ false };
	bool supportsSourceRequest{ true };
	std::string launchRequestType{ "launch" };
	json launchArgs;
	std::string locale{ "en-US" };

	ProtocolSettings() = default;

	ProtocolSettings( const nlohmann::json& configuration );
};

} // namespace ecode
