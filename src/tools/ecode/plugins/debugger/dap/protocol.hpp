#pragma once

#include <eepp/core/containers.hpp>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ecode::dap {

struct Message {
	/**
	 * @brief id unique identifier for the message
	 */
	int id;
	/**
	 * @brief format A format string for the message.
	 *
	 * Embedded variables have the form '{name}'. If variable
	 * name starts with an underscore character, the variable
	 * does not contain user data (PII) and can be safely used for
	 * telemetry purposes.
	 */
	std::string format;
	/**
	 * @brief variables An object used as a dictionary for looking
	 * up the variables in the format string.
	 */
	std::optional<std::unordered_map<std::string, std::string>> variables;
	/**
	 * @brief sendTelemetry If true send telemetry
	 */
	std::optional<bool> sendTelemetry;
	/**
	 * @brief showUser If true show user
	 */
	std::optional<bool> showUser;
	/**
	 * @brief showUser An optional url where additional information
	 * about this message can be found.
	 */
	std::optional<std::string> url;
	/**
	 * @brief urlLabel An optional label that is presented to the user as the UI
	 * for opening the url.
	 */
	std::optional<std::string> urlLabel;

	Message() = default;
	Message( const json& body );
};

struct Response {
	int request_seq;
	bool success;
	std::string command;
	std::string message;
	json body;
	std::optional<Message> errorBody;

	Response() = default;
	Response( const json& msg );

	bool isCancelled() const;
};

struct ProcessInfo {
	/**
	 * @brief name the logical name of the process
	 *
	 * This is usually the full path to process's executable file.
	 */
	std::string name;
	/**
	 * @brief systemProcessId the system process id of the debugged process
	 *
	 * This property will be missing for non-system processes.
	 */
	std::optional<int> systemProcessId;
	/**
	 * @brief isLocalProcess if true, the process is running on the same computer as DA
	 */
	std::optional<bool> isLocalProcess;
	/**
	 * @brief startMethod describes how the debug engine started debugging this process.
	 */
	std::optional<std::string> startMethod;
	/**
	 * @brief pointerSize the sized of a pointer or address for this process, in bits.
	 *
	 * This value may be used by clients when formatting addresses for display.
	 */
	std::optional<int> pointerSize;

	ProcessInfo() = default;
	ProcessInfo( const json& body );
};

struct Checksum {
	std::string checksum;
	std::string algorithm;

	Checksum() = default;
	Checksum( const json& body );

	json toJson() const;
};

struct Source {
	std::string name;
	std::string path;
	std::optional<int> sourceReference;
	std::optional<std::string> presentationHint;
	std::string origin;
	std::vector<Source> sources;
	json adapterData;
	std::vector<Checksum> checksums;

	std::string unifiedId() const;
	static std::string getUnifiedId( const std::string& path, std::optional<int> sourceReference );

	Source() = default;
	Source( const json& body );
	Source( const std::string& path );

	json toJson() const;
};

struct SourceContent {
	std::string content;
	std::optional<std::string> mimeType;

	SourceContent() = default;
	SourceContent( const json& body );
	SourceContent( const std::string& path );
};

struct SourceBreakpoint {
	int line;
	std::optional<int> column;
	/**
	 * An optional expression for conditional breakpoints.
	 * It is only honored by a debug adapter if the capability
	 * 'supportsConditionalBreakpoints' is true.
	 */
	std::optional<std::string> condition;
	/**
	 * An optional expression that controls how many hits of the breakpoint are
	 * ignored.
	 * The backend is expected to interpret the expression as needed.
	 * The attribute is only honored by a debug adapter if the capability
	 * 'supportsHitConditionalBreakpoints' is true.
	 */
	std::optional<std::string> hitCondition;
	/**
	 * If this attribute exists and is non-empty, the backend must not 'break'
	 * (stop)
	 * but log the message instead. Expressions within {} are interpolated.
	 * The attribute is only honored by a debug adapter if the capability
	 * 'supportsLogPoints' is true.
	 */
	std::optional<std::string> logMessage;

	SourceBreakpoint() = default;
	SourceBreakpoint( const json& body );
	SourceBreakpoint( const int line );

	json toJson() const;

	bool operator==( const SourceBreakpoint& other ) const { return line == other.line; }
};

struct SourceBreakpointStateful : public SourceBreakpoint {
	bool enabled{ true };

	SourceBreakpointStateful() = default;
	SourceBreakpointStateful( const json& body ) : SourceBreakpoint( body ) {}
	SourceBreakpointStateful( const int line ) : SourceBreakpoint( line ) {}

	bool operator==( const SourceBreakpointStateful& other ) const { return line == other.line; }
};

struct Breakpoint {
	/**
	 * An optional identifier for the breakpoint. It is needed if breakpoint
	 * events are used to update or remove breakpoints.
	 */
	std::optional<int> id;
	/**
	 * If true breakpoint could be set (but not necessarily at the desired
	 * location).
	 */
	bool verified;
	/**
	 * An optional message about the state of the breakpoint.
	 * This is shown to the user and can be used to explain why a breakpoint could
	 * not be verified.
	 */
	std::optional<std::string> message;
	std::optional<Source> source;
	/**
	 * The start line of the actual range covered by the breakpoint.
	 */
	std::optional<int> line;
	std::optional<int> column;
	std::optional<int> endLine;
	std::optional<int> endColumn;
	/**
	 * An optional memory reference to where the breakpoint is set.
	 */
	std::optional<std::string> instructionReference;
	/**
	 * An optional offset from the instruction reference.
	 * This can be negative.
	 */
	std::optional<int> offset;

	Breakpoint() = default;
	Breakpoint( const json& body );
	Breakpoint( const int line );
};

class Output {
  public:
	enum class Category { Console, Important, Stdout, Stderr, Telemetry, Unknown };

	enum class Group {
		Start,
		StartCollapsed,
		End,
	};

	Category category;
	std::string output;
	std::optional<Group> group;
	std::optional<int> variablesReference;
	std::optional<Source> source;
	std::optional<int> line;
	std::optional<int> column;
	json data;

	Output() = default;
	Output( const json& body );
	Output( const std::string& output, const Category& category );

	bool isSpecialOutput() const;
};

struct Capabilities {
	bool supportsConfigurationDoneRequest;
	bool supportsFunctionBreakpoints;
	bool supportsConditionalBreakpoints;
	bool supportsHitConditionalBreakpoints;
	bool supportsLogPoints;
	bool supportsModulesRequest;
	bool supportsTerminateRequest;
	bool supportTerminateDebuggee;
	bool supportsGotoTargetsRequest;

	Capabilities() = default;
	Capabilities( const json& body );
};

struct ThreadEvent {
	std::string reason;
	int threadId;

	ThreadEvent() = default;
	ThreadEvent( const json& body );
};

struct Module {
	std::optional<int> id_int;
	std::optional<std::string> id_str;
	std::string name;
	std::optional<std::string> path;
	std::optional<bool> isOptimized;
	std::optional<bool> isUserCode;
	std::optional<std::string> version;
	std::optional<std::string> symbolStatus;
	std::optional<std::string> symbolFilePath;
	std::optional<std::string> dateTimeStamp;
	std::optional<std::string> addressRange;

	Module() = default;
	Module( const json& body );
};

struct ModulesInfo {
	std::vector<Module> modules;
	std::optional<int> totalModules;

	ModulesInfo() = default;
	ModulesInfo( const json& body );
};

struct ModuleEvent {
	std::string reason;
	Module module;

	ModuleEvent() = default;
	ModuleEvent( const json& body );
};

struct StoppedEvent {
	std::string reason;
	std::optional<std::string> description;
	/**
	 * @brief threadId The thread which was stopped.
	 */
	std::optional<int> threadId;
	/**
	 * @brief preserverFocusHint A value of true hints to the frontend that
	 * this event should not change the focus
	 */
	std::optional<bool> preserveFocusHint;
	/**
	 * @brief text Additional information.
	 */
	std::optional<std::string> text;
	/**
	 * @brief allThreadsStopped if true, a DA can announce
	 * that all threads have stopped.
	 * - The client should use this information to enable that all threads can be
	 *  expanded to access their stacktraces.
	 * - If the attribute is missing or false, only the thread with the given threadId
	 *   can be expanded.
	 */
	std::optional<bool> allThreadsStopped;
	/**
	 * @brief hitBreakpointsIds ids of the breakpoints that triggered the event
	 */
	std::optional<std::vector<int>> hitBreakpointsIds;

	StoppedEvent() = default;
	StoppedEvent( const json& body );
};

struct ContinuedEvent {
	int threadId;
	/**
	 * If 'allThreadsContinued' is true, a debug adapter can announce that all
	 * threads have continued.
	 */
	std::optional<bool> allThreadsContinued;

	ContinuedEvent() = default;
	ContinuedEvent( int threadId, bool allThreadsContinued );
	ContinuedEvent( const json& body );
};

struct BreakpointEvent {
	std::string reason;
	Breakpoint breakpoint;

	BreakpointEvent() = default;
	BreakpointEvent( const json& body );
};

struct Thread {
	int id;
	std::string name;

	Thread() = default;
	Thread( const json& body );
	explicit Thread( const int id );

	static std::vector<Thread> parseList( const json& threads );
};

struct StackFrame {
	int id;
	std::string name;
	std::optional<Source> source;
	int line;
	int column;
	std::optional<int> endLine;
	std::optional<int> endColumn;
	std::optional<bool> canRestart;
	std::optional<std::string> instructionPointerReference;
	std::optional<int> moduleId_int;
	std::optional<std::string> moduleId_str;
	std::optional<std::string> presentationHint;

	StackFrame() = default;
	StackFrame( const json& body );
};

struct StackTraceInfo {
	std::vector<StackFrame> stackFrames;
	std::optional<int> totalFrames;

	StackTraceInfo() = default;
	StackTraceInfo( const json& body );
};

struct Scope {
	std::string name;
	std::optional<std::string> presentationHint;
	int variablesReference;
	std::optional<int> namedVariables;
	std::optional<int> indexedVariables;
	std::optional<bool> expensive;
	std::optional<Source> source;
	std::optional<int> line;
	std::optional<int> column;
	std::optional<int> endLine;
	std::optional<int> endColumn;

	Scope() = default;
	Scope( const json& body );
	Scope( int variablesReference, std::string name );

	static std::vector<Scope> parseList( const json& scopes );
};

struct Variable {
	enum Type { Indexed = 1, Named = 2, Both = 3 };

	std::string name;
	std::string value;
	std::optional<std::string> type;
	/**
	 * @brief evaluateName Optional evaluatable name of tihs variable which can be
	 * passed to the EvaluateRequest to fetch the variable's value
	 */
	std::optional<std::string> evaluateName;
	/**
	 * @brief variablesReference if > 0, its children can be retrieved by VariablesRequest
	 */
	int variablesReference;
	/**
	 * @brief namedVariables number of named child variables
	 */
	std::optional<int> namedVariables;
	/**
	 * @brief indexedVariables number of indexed child variables
	 */
	std::optional<int> indexedVariables;
	/**
	 * @brief memoryReference optional memory reference for the variable if the
	 * variable represents executable code, such as a function pointer.
	 * Requires 'supportsMemoryReferences'
	 */
	std::optional<std::string> memoryReference;

	/**
	 * the value has changed since the last time
	 */
	std::optional<bool> valueChanged;

	Variable() = default;
	Variable( const json& body );
	Variable( const std::string& name, const std::string& value, const int reference = 0 );

	static std::vector<Variable> parseList( const json& variables );
};

struct EvaluateInfo {
	std::string result;
	std::optional<std::string> type;
	int variablesReference;
	std::optional<int> namedVariables;
	std::optional<int> indexedVariables;
	std::optional<std::string> memoryReference;

	EvaluateInfo();
	EvaluateInfo( const json& body );
};

struct GotoTarget {
	int id;
	std::string label;
	int line;
	std::optional<int> column;
	std::optional<int> endLine;
	std::optional<int> endColumn;
	std::optional<std::string> instructionPointerReference;

	GotoTarget() = default;
	GotoTarget( const json& body );

	static std::vector<GotoTarget> parseList( const json& variables );
};

} // namespace ecode::dap

template <> struct std::hash<ecode::dap::SourceBreakpoint> {
	std::size_t operator()( ecode::dap::SourceBreakpoint const& breakpoint ) const noexcept {
		size_t h1 = std::hash<int>()( breakpoint.line );
		size_t h2 = breakpoint.column ? std::hash<int>()( *breakpoint.column ) : 0;
		size_t h3 = breakpoint.condition ? std::hash<std::string>()( *breakpoint.condition ) : 0;
		size_t h4 =
			breakpoint.hitCondition ? std::hash<std::string>()( *breakpoint.hitCondition ) : 0;
		size_t h5 = breakpoint.logMessage ? std::hash<std::string>()( *breakpoint.logMessage ) : 0;
		return hashCombine( h1, h2, h3, h4, h5 );
	}
};

template <> struct std::hash<ecode::dap::SourceBreakpointStateful> {
	std::size_t
	operator()( ecode::dap::SourceBreakpointStateful const& breakpoint ) const noexcept {
		size_t h1 = std::hash<int>()( breakpoint.line );
		size_t h2 = breakpoint.column ? std::hash<int>()( *breakpoint.column ) : 0;
		size_t h3 = breakpoint.condition ? std::hash<std::string>()( *breakpoint.condition ) : 0;
		size_t h4 =
			breakpoint.hitCondition ? std::hash<std::string>()( *breakpoint.hitCondition ) : 0;
		size_t h5 = breakpoint.logMessage ? std::hash<std::string>()( *breakpoint.logMessage ) : 0;
		size_t h6 = std::hash<bool>()( breakpoint.enabled );
		return hashCombine( h1, h2, h3, h4, h5, h6 );
	}
};
