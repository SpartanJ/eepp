#pragma once

#include <eepp/core/containers.hpp>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ecode { namespace acp {

json parseLegacyConfigOptions( const json& body, json configOptions,
							   const std::string& forceId = "",
							   const std::string& forceValue = "" );

struct ClientCapabilities {
	bool terminal{ false };
	bool fsReadTextFile{ false };
	bool fsWriteTextFile{ false };

	ClientCapabilities() = default;
	ClientCapabilities( const json& body );
	json toJson() const;
};

struct AgentCapabilities {
	bool loadSession{ false };
	json mcpCapabilities;
	json promptCapabilities;
	json sessionCapabilities;

	AgentCapabilities() = default;
	AgentCapabilities( const json& body );
};

struct InitializeRequest {
	int protocolVersion{ 1 };
	ClientCapabilities clientCapabilities;

	InitializeRequest() = default;
	json toJson() const;
};

struct InitializeResponse {
	int protocolVersion{ 1 };
	AgentCapabilities agentCapabilities;

	InitializeResponse() = default;
	InitializeResponse( const json& body );
};

struct NewSessionRequest {
	std::string cwd;
	json mcpServers;

	NewSessionRequest() = default;
	json toJson() const;
};

struct NewSessionResponse {
	std::string sessionId;
	json configOptions;

	NewSessionResponse() = default;
	NewSessionResponse( const json& body );
};

struct LoadSessionRequest {
	std::string sessionId;
	std::string cwd;
	json mcpServers;

	LoadSessionRequest() = default;
	json toJson() const;
};

struct LoadSessionResponse {
	json configOptions;

	LoadSessionResponse() = default;
	LoadSessionResponse( const json& body );
};

struct SetConfigOptionRequest {
	std::string sessionId;
	std::string configId;
	std::string optionId;
	std::string type;
	json value;

	SetConfigOptionRequest() = default;
	json toJson() const;
};

struct SetConfigOptionResponse {
	json configOptions;

	SetConfigOptionResponse() = default;
	SetConfigOptionResponse( const json& body, const std::string& configId = "",
							 const std::string& optionId = "" );
};

struct SessionInfo {
	std::string sessionId;
	std::string cwd;
	std::string title;
	std::string updatedAt;

	SessionInfo() = default;
	SessionInfo( const json& body );
};

struct ListSessionsRequest {
	std::string cursor;
	std::string cwd;

	ListSessionsRequest() = default;
	json toJson() const;
};

struct ListSessionsResponse {
	std::string nextCursor;
	std::vector<SessionInfo> sessions;

	ListSessionsResponse() = default;
	ListSessionsResponse( const json& body );
};

struct ResponseError {
	int code{ 0 };
	std::string message;
	json data;

	ResponseError() = default;
	ResponseError( int code, std::string message, json data = {} ) :
		code( code ), message( std::move( message ) ), data( std::move( data ) ) {}
	ResponseError( const json& body ) {
		if ( body.contains( "code" ) )
			code = body["code"].get<int>();
		if ( body.contains( "message" ) )
			message = body["message"].get<std::string>();
		if ( body.contains( "data" ) )
			data = body["data"];
	}
};

struct PromptRequest {
	std::string sessionId;
	json prompt; // Array of ContentBlock

	PromptRequest() = default;
	json toJson() const;
};

struct PromptResponse {
	std::string stopReason;

	PromptResponse() = default;
	PromptResponse( const json& body );
};

struct ToolCallLocation {
	std::string path;
	std::optional<int> line;

	ToolCallLocation() = default;
	ToolCallLocation( const json& body );
};

struct ToolCall {
	std::string title;
	std::string toolCallId;
	std::string kind;
	json rawInput;
	json rawOutput;
	std::vector<ToolCallLocation> locations;

	ToolCall() = default;
	ToolCall( const json& body );
};

struct ReadTextFileRequest {
	std::string sessionId;
	std::string path;
	std::optional<int> line;
	std::optional<int> limit;

	ReadTextFileRequest() = default;
	ReadTextFileRequest( const json& body );
};

struct ReadTextFileResponse {
	std::string content;

	ReadTextFileResponse() = default;
	json toJson() const;
};

struct WriteTextFileRequest {
	std::string sessionId;
	std::string path;
	std::string content;

	WriteTextFileRequest() = default;
	WriteTextFileRequest( const json& body );
};

struct WriteTextFileResponse {
	WriteTextFileResponse() = default;
	json toJson() const;
};

struct EnvVariable {
	std::string name;
	std::string value;
	EnvVariable() = default;
	EnvVariable( const json& body );
	json toJson() const;
};

struct CreateTerminalRequest {
	std::string sessionId;
	std::string command;
	std::vector<std::string> args;
	std::optional<std::string> cwd;
	std::vector<EnvVariable> env;
	std::optional<uint64_t> outputByteLimit;

	CreateTerminalRequest() = default;
	CreateTerminalRequest( const json& body );
};

struct CreateTerminalResponse {
	std::string terminalId;

	CreateTerminalResponse() = default;
	json toJson() const;
};

struct TerminalExitStatus {
	std::optional<uint32_t> exitCode;
	std::optional<std::string> signal;

	TerminalExitStatus() = default;
	TerminalExitStatus( const json& body );
	json toJson() const;
};

struct TerminalOutputRequest {
	std::string sessionId;
	std::string terminalId;
	std::optional<uint64_t> outputByteLimit;

	TerminalOutputRequest() = default;
	TerminalOutputRequest( const json& body );
};

struct TerminalOutputResponse {
	std::string output;
	bool truncated{ false };
	std::optional<TerminalExitStatus> exitStatus;

	TerminalOutputResponse() = default;
	json toJson() const;
};

struct KillTerminalRequest {
	std::string sessionId;
	std::string terminalId;

	KillTerminalRequest() = default;
	KillTerminalRequest( const json& body );
};

struct KillTerminalResponse {
	KillTerminalResponse() = default;
	json toJson() const;
};

struct ReleaseTerminalRequest {
	std::string sessionId;
	std::string terminalId;

	ReleaseTerminalRequest() = default;
	ReleaseTerminalRequest( const json& body );
};

struct ReleaseTerminalResponse {
	ReleaseTerminalResponse() = default;
	json toJson() const;
};

struct WaitForTerminalExitRequest {
	std::string sessionId;
	std::string terminalId;

	WaitForTerminalExitRequest() = default;
	WaitForTerminalExitRequest( const json& body );
};

struct WaitForTerminalExitResponse {
	std::optional<uint32_t> exitCode;
	std::optional<std::string> signal;

	WaitForTerminalExitResponse() = default;
	json toJson() const;
};

struct PermissionOption {
	std::string optionId;
	std::string name;
	std::string kind;

	PermissionOption() = default;
	PermissionOption( const json& body );
	json toJson() const;
};

struct RequestPermissionRequest {
	std::string sessionId;
	ToolCall toolCall;
	std::vector<PermissionOption> options;

	RequestPermissionRequest() = default;
	RequestPermissionRequest( const json& body );
};

struct RequestPermissionResponse {
	std::string outcome;
	std::optional<std::string> optionId;

	RequestPermissionResponse() = default;
	json toJson() const;
};

}} // namespace ecode::acp
