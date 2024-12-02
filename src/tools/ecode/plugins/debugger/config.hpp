#pragma once

#include <eepp/core/containers.hpp>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace EE;

namespace ecode {

struct Command {
	std::string command;
	std::vector<std::string> arguments;
	std::unordered_map<std::string, std::string> environment;
};

struct Connection {
	int port;
	std::string host;
};

struct BusSettings {
	std::optional<Command> command;
	std::optional<Connection> connection;

	bool isValid() const;
	bool hasCommand() const;
	bool hasConnection() const;
};

struct ProtocolSettings {
	bool linesStartAt1;
	bool columnsStartAt1;
	bool pathFormatURI;
	bool redirectStderr;
	bool redirectStdout;
	bool supportsSourceRequest;
	json launchRequest;
	std::string locale;
};

} // namespace ecode
