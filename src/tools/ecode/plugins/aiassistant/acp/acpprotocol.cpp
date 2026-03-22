#include "acpprotocol.hpp"

namespace ecode { namespace acp {

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
	return j;
}

}} // namespace ecode::acp
