#include "acpprotocol.hpp"

namespace ecode { namespace acp {

json parseLegacyConfigOptions( const json& body, json configOptions, const std::string& forceId,
							   const std::string& forceValue ) {
	if ( configOptions.is_null() ) {
		configOptions = json::array();
	}

	auto updateOrAdd = [&configOptions]( json newOpt ) {
		bool found = false;
		for ( auto& opt : configOptions ) {
			if ( opt.is_object() && opt.contains( "id" ) && opt["id"] == newOpt["id"] ) {
				opt = std::move( newOpt );
				found = true;
				break;
			}
		}
		if ( !found ) {
			configOptions.push_back( std::move( newOpt ) );
		}
	};

	if ( body.contains( "models" ) && body["models"].is_object() ) {
		auto models = body["models"];
		if ( models.contains( "availableModels" ) && models["availableModels"].is_array() ) {
			json modelConfig = { { "id", "model" },
								 { "name", "Model" },
								 { "type", "select" },
								 { "category", "model" },
								 { "options", json::array() } };
			for ( const auto& model : models["availableModels"] ) {
				modelConfig["options"].push_back( { { "id", model.value( "modelId", "" ) },
													{ "name", model.value( "name", "" ) } } );
			}
			if ( models.contains( "currentModelId" ) ) {
				modelConfig["currentValue"] = models.value( "currentModelId", "" );
				modelConfig["default"] = models.value( "currentModelId", "" );
			} else if ( !modelConfig["options"].empty() ) {
				modelConfig["currentValue"] = modelConfig["options"][0]["id"];
				modelConfig["default"] = modelConfig["options"][0]["id"];
			}
			updateOrAdd( std::move( modelConfig ) );
		}
	}

	if ( body.contains( "modes" ) && body["modes"].is_object() ) {
		auto modes = body["modes"];
		if ( modes.contains( "availableModes" ) && modes["availableModes"].is_array() ) {
			json modeConfig = { { "id", "mode" },
								{ "name", "Mode" },
								{ "type", "select" },
								{ "category", "mode" },
								{ "options", json::array() } };
			for ( const auto& mode : modes["availableModes"] ) {
				modeConfig["options"].push_back(
					{ { "id", mode.value( "id", "" ) }, { "name", mode.value( "name", "" ) } } );
			}
			if ( modes.contains( "currentModeId" ) ) {
				modeConfig["currentValue"] = modes.value( "currentModeId", "" );
				modeConfig["default"] = modes.value( "currentModeId", "" );
			} else if ( !modeConfig["options"].empty() ) {
				modeConfig["currentValue"] = modeConfig["options"][0]["id"];
				modeConfig["default"] = modeConfig["options"][0]["id"];
			}
			updateOrAdd( std::move( modeConfig ) );
		}
	}

	if ( !forceId.empty() ) {
		for ( auto& opt : configOptions ) {
			if ( opt.is_object() && opt.contains( "id" ) && opt["id"] == forceId ) {
				opt["currentValue"] = forceValue;
				break;
			}
		}
	}

	return configOptions;
}

ClientCapabilities::ClientCapabilities( const json& body ) {
	if ( body.contains( "terminal" ) )
		terminal = body.value( "terminal", false );
	if ( body.contains( "fs" ) ) {
		auto fs = body["fs"];
		if ( fs.contains( "readTextFile" ) )
			fsReadTextFile = fs.value( "readTextFile", false );
		if ( fs.contains( "writeTextFile" ) )
			fsWriteTextFile = fs.value( "writeTextFile", false );
	}
}

json ClientCapabilities::toJson() const {
	return {
		{ "terminal", terminal },
		{ "fs", { { "readTextFile", fsReadTextFile }, { "writeTextFile", fsWriteTextFile } } } };
}

AgentCapabilities::AgentCapabilities( const json& body ) {
	if ( body.contains( "loadSession" ) )
		loadSession = body.value( "loadSession", false );
	if ( body.contains( "mcpCapabilities" ) )
		mcpCapabilities = body["mcpCapabilities"];
	if ( body.contains( "promptCapabilities" ) )
		promptCapabilities = body["promptCapabilities"];
	if ( body.contains( "sessionCapabilities" ) )
		sessionCapabilities = body["sessionCapabilities"];
}

json InitializeRequest::toJson() const {
	return { { "protocolVersion", protocolVersion },
			 { "clientCapabilities", clientCapabilities.toJson() } };
}

InitializeResponse::InitializeResponse( const json& body ) {
	if ( body.contains( "protocolVersion" ) )
		protocolVersion = body.value( "protocolVersion", 1 );
	if ( body.contains( "agentCapabilities" ) )
		agentCapabilities = AgentCapabilities( body["agentCapabilities"] );
}

json NewSessionRequest::toJson() const {
	json j = { { "cwd", cwd } };
	if ( mcpServers.is_null() ) {
		j["mcpServers"] = json::array();
	} else {
		j["mcpServers"] = mcpServers;
	}
	return j;
}

NewSessionResponse::NewSessionResponse( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "configOptions" ) )
		configOptions = body["configOptions"];
	configOptions = parseLegacyConfigOptions( body, configOptions );
}

json LoadSessionRequest::toJson() const {
	json j = { { "sessionId", sessionId }, { "cwd", cwd } };
	if ( mcpServers.is_null() ) {
		j["mcpServers"] = json::array();
	} else {
		j["mcpServers"] = mcpServers;
	}
	return j;
}

LoadSessionResponse::LoadSessionResponse( const json& body ) {
	if ( body.contains( "configOptions" ) )
		configOptions = body["configOptions"];
	configOptions = parseLegacyConfigOptions( body, configOptions );
}

json SetConfigOptionRequest::toJson() const {
	json j = { { "sessionId", sessionId }, { "configId", configId }, { "optionId", optionId } };
	if ( !type.empty() )
		j["type"] = type;
	if ( !value.is_null() )
		j["value"] = value;
	return j;
}

SetConfigOptionResponse::SetConfigOptionResponse( const json& body, const std::string& configId,
												  const std::string& optionId ) {
	if ( body.contains( "configOptions" ) )
		configOptions = body["configOptions"];
	configOptions = parseLegacyConfigOptions( body, configOptions, configId, optionId );
}

SessionInfo::SessionInfo( const json& body ) {
	if ( body.contains( "sessionId" ) && body["sessionId"].is_string() )
		sessionId = body["sessionId"].get<std::string>();
	if ( body.contains( "cwd" ) && body["cwd"].is_string() )
		cwd = body["cwd"].get<std::string>();
	if ( body.contains( "title" ) && body["title"].is_string() )
		title = body["title"].get<std::string>();
	if ( body.contains( "updatedAt" ) && body["updatedAt"].is_string() )
		updatedAt = body["updatedAt"].get<std::string>();
}

json ListSessionsRequest::toJson() const {
	json j = json::object();
	if ( !cursor.empty() )
		j["cursor"] = cursor;
	if ( !cwd.empty() )
		j["cwd"] = cwd;
	return j;
}

ListSessionsResponse::ListSessionsResponse( const json& body ) {
	if ( body.contains( "nextCursor" ) && body["nextCursor"].is_string() )
		nextCursor = body["nextCursor"].get<std::string>();
	if ( body.contains( "sessions" ) && body["sessions"].is_array() ) {
		for ( const auto& item : body["sessions"] ) {
			sessions.emplace_back( SessionInfo( item ) );
		}
	}
}

json PromptRequest::toJson() const {
	return { { "sessionId", sessionId }, { "prompt", prompt } };
}

PromptResponse::PromptResponse( const json& body ) {
	if ( body.contains( "stopReason" ) && body["stopReason"].is_string() )
		stopReason = body.value( "stopReason", "" );
}

ToolCallLocation::ToolCallLocation( const json& body ) {
	if ( body.contains( "path" ) )
		path = body.value( "path", "" );
	if ( body.contains( "line" ) && !body["line"].is_null() )
		line = body["line"].get<int>();
}

ToolCall::ToolCall( const json& body ) {
	if ( body.contains( "title" ) )
		title = body.value( "title", "" );
	if ( body.contains( "toolCallId" ) )
		toolCallId = body.value( "toolCallId", "" );
	if ( body.contains( "kind" ) && body["kind"].is_string() )
		kind = body.value( "kind", "" );
	if ( body.contains( "rawInput" ) )
		rawInput = body["rawInput"];
	if ( body.contains( "rawOutput" ) )
		rawOutput = body["rawOutput"];
	if ( body.contains( "locations" ) && body["locations"].is_array() ) {
		for ( const auto& l : body["locations"] ) {
			locations.push_back( ToolCallLocation( l ) );
		}
	}
}

ReadTextFileRequest::ReadTextFileRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "path" ) )
		path = body.value( "path", "" );
	if ( body.contains( "line" ) && !body["line"].is_null() )
		line = body["line"].get<int>();
	if ( body.contains( "limit" ) && !body["limit"].is_null() )
		limit = body["limit"].get<int>();
}

json ReadTextFileResponse::toJson() const {
	return { { "content", content } };
}

WriteTextFileRequest::WriteTextFileRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "path" ) )
		path = body.value( "path", "" );
	if ( body.contains( "content" ) )
		content = body.value( "content", "" );
}

json WriteTextFileResponse::toJson() const {
	return json::object();
}

EnvVariable::EnvVariable( const json& body ) {
	if ( body.contains( "name" ) )
		name = body.value( "name", "" );
	if ( body.contains( "value" ) )
		value = body.value( "value", "" );
}

json EnvVariable::toJson() const {
	return { { "name", name }, { "value", value } };
}

CreateTerminalRequest::CreateTerminalRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "command" ) )
		command = body.value( "command", "" );
	if ( body.contains( "args" ) && body["args"].is_array() ) {
		for ( const auto& a : body["args"] )
			args.push_back( a.get<std::string>() );
	}
	if ( body.contains( "cwd" ) && !body["cwd"].is_null() )
		cwd = body["cwd"].get<std::string>();
	if ( body.contains( "env" ) && body["env"].is_array() ) {
		for ( const auto& e : body["env"] )
			env.push_back( EnvVariable( e ) );
	}
	if ( body.contains( "outputByteLimit" ) && !body["outputByteLimit"].is_null() )
		outputByteLimit = body["outputByteLimit"].get<uint64_t>();
}

json CreateTerminalResponse::toJson() const {
	return { { "terminalId", terminalId } };
}

TerminalExitStatus::TerminalExitStatus( const json& body ) {
	if ( body.contains( "exitCode" ) && !body["exitCode"].is_null() )
		exitCode = body["exitCode"].get<uint32_t>();
	if ( body.contains( "signal" ) && !body["signal"].is_null() )
		signal = body["signal"].get<std::string>();
}

json TerminalExitStatus::toJson() const {
	json j;
	if ( exitCode )
		j["exitCode"] = *exitCode;
	else
		j["exitCode"] = nullptr;
	if ( signal )
		j["signal"] = *signal;
	else
		j["signal"] = nullptr;
	return j;
}

TerminalOutputRequest::TerminalOutputRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "terminalId" ) )
		terminalId = body.value( "terminalId", "" );
	if ( body.contains( "outputByteLimit" ) && !body["outputByteLimit"].is_null() )
		outputByteLimit = body["outputByteLimit"].get<uint64_t>();
}

json TerminalOutputResponse::toJson() const {
	json j = { { "output", output }, { "truncated", truncated } };
	if ( exitStatus )
		j["exitStatus"] = exitStatus->toJson();
	return j;
}

KillTerminalRequest::KillTerminalRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "terminalId" ) )
		terminalId = body.value( "terminalId", "" );
}

json KillTerminalResponse::toJson() const {
	return json::object();
}

ReleaseTerminalRequest::ReleaseTerminalRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "terminalId" ) )
		terminalId = body.value( "terminalId", "" );
}

json ReleaseTerminalResponse::toJson() const {
	return json::object();
}

WaitForTerminalExitRequest::WaitForTerminalExitRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "terminalId" ) )
		terminalId = body.value( "terminalId", "" );
}

json WaitForTerminalExitResponse::toJson() const {
	json j;
	if ( exitCode )
		j["exitCode"] = *exitCode;
	else
		j["exitCode"] = nullptr;
	if ( signal )
		j["signal"] = *signal;
	else
		j["signal"] = nullptr;
	return j;
}

PermissionOption::PermissionOption( const json& body ) {
	if ( body.contains( "optionId" ) )
		optionId = body.value( "optionId", "" );
	if ( body.contains( "name" ) )
		name = body.value( "name", "" );
	if ( body.contains( "kind" ) )
		kind = body.value( "kind", "" );
}

json PermissionOption::toJson() const {
	return { { "optionId", optionId }, { "name", name }, { "kind", kind } };
}

RequestPermissionRequest::RequestPermissionRequest( const json& body ) {
	if ( body.contains( "sessionId" ) )
		sessionId = body.value( "sessionId", "" );
	if ( body.contains( "toolCall" ) )
		toolCall = ToolCall( body["toolCall"] );
	if ( body.contains( "options" ) && body["options"].is_array() ) {
		for ( const auto& o : body["options"] )
			options.push_back( PermissionOption( o ) );
	}
}

json RequestPermissionResponse::toJson() const {
	json j = { { "outcome", outcome } };
	if ( optionId )
		j["optionId"] = *optionId;
	return { { "outcome", j } };
}

}} // namespace ecode::acp
