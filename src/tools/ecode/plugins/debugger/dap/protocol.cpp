#include "protocol.hpp"
#include "messages.hpp"
#include <eepp/core/core.hpp>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/filesystem.hpp>

using namespace EE;
using namespace EE::System;

std::optional<int> parseOptionalInt( const json& value, const std::string_view& name ) {
	if ( !value.contains( name ) || value[name].is_null() || value[name].empty() ||
		 !value[name].is_number() ) {
		return std::nullopt;
	}
	return value[name].get<int>();
}

std::optional<bool> parseOptionalBool( const json& value, const std::string_view& name ) {
	if ( !value.contains( name ) || value[name].is_null() || value[name].empty() ||
		 !value[name].is_boolean() ) {
		return std::nullopt;
	}
	return value[name].get<bool>();
}

std::optional<std::string> parseOptionalString( const json& value, const std::string_view& name ) {
	if ( !value.contains( name ) || value[name].is_null() || value[name].empty() ||
		 !value[name].is_string() ) {
		return std::nullopt;
	}
	return value[name].get<std::string>();
}

template <typename T>
std::optional<T> parseOptionalObject( const json& value, const std::string_view& name ) {
	if ( !value.contains( name ) || value[name].is_null() || value[name].empty() ||
		 !value[name].is_object() ) {
		return std::nullopt;
	}
	return T( value[name] );
}

std::optional<std::unordered_map<std::string, std::string>>
parseOptionalStringMap( const json& value, const std::string_view& name ) {
	if ( !value.contains( name ) || value[name].is_null() || value[name].empty() ||
		 !value[name].is_object() ) {
		return std::nullopt;
	}
	const auto& dict = value[name];
	std::unordered_map<std::string, std::string> map;
	for ( auto it = dict.begin(); it != dict.end(); ++it ) {
		map[it.key()] = it.value().get<std::string>();
	}
	return map;
}

template <typename T> std::vector<T> parseObjectList( const json& array ) {
	std::vector<T> out;
	for ( const auto& item : array ) {
		out.emplace_back( T( item ) );
	}
	return out;
}

std::optional<std::vector<int>> parseOptionalIntList( const json& value,
													  const std::string_view& name ) {
	if ( !value.contains( name ) || value[name].is_null() || value[name].empty() ||
		 !value[name].is_array() ) {
		return std::nullopt;
	}
	std::vector<int> values;
	for ( const auto& item : value[name] ) {
		values.push_back( item.get<int>() );
	}
	return values;
}

template <typename T> json toJsonArray( const std::vector<T>& items ) {
	json out = json::array();
	for ( const auto& item : items ) {
		out.emplace_back( item.toJson() );
	}
	return out;
}

namespace ecode::dap {

Message::Message( const json& body ) :
	id( body[DAP_ID].get<int>() ),
	format( body["format"].get<std::string>() ),
	variables( parseOptionalStringMap( body, "variables" ) ),
	sendTelemetry( parseOptionalBool( body, "sendTelemetry" ) ),
	showUser( parseOptionalBool( body, "showUser" ) ),
	url( parseOptionalString( body, "url" ) ),
	urlLabel( parseOptionalString( body, "urlLabel" ) ) {}

Response::Response( const json& msg ) :
	request_seq( msg.value( "request_seq", -1 ) ),
	success( msg.value( "success", false ) ),
	command( msg.value( DAP_COMMAND, "" ) ),
	message( msg.value( "message", "" ) ),
	body( msg.contains( DAP_BODY ) ? msg[DAP_BODY] : nlohmann::json{} ),
	errorBody( success ? std::nullopt : parseOptionalObject<Message>( body, "error" ) ) {}

bool Response::isCancelled() const {
	return message == "cancelled";
}

ProcessInfo::ProcessInfo( const json& body ) :
	name( body.value( DAP_NAME, "" ) ),
	systemProcessId( parseOptionalInt( body, DAP_SYSTEM_PROCESS_ID ) ),
	isLocalProcess( parseOptionalBool( body, DAP_IS_LOCAL_PROCESS ) ),
	startMethod( parseOptionalString( body, DAP_START_METHOD ) ),
	pointerSize( parseOptionalInt( body, DAP_POINTER_SIZE ) ) {}

Output::Output( const json& body ) :
	category( Category::Unknown ),
	output( body.value( DAP_OUTPUT, "" ) ),
	group( std::nullopt ),
	variablesReference( parseOptionalInt( body, DAP_VARIABLES_REFERENCE ) ),
	source( parseOptionalObject<Source>( body, DAP_SOURCE ) ),
	line( parseOptionalInt( body, DAP_LINE ) ),
	column( parseOptionalInt( body, DAP_COLUMN ) ),
	data( body.contains( DAP_DATA ) ? body[DAP_DATA] : nlohmann::json{} ) {
	if ( body.contains( DAP_GROUP ) ) {
		const auto value = body[DAP_GROUP].get<std::string>();
		if ( DAP_START == value ) {
			group = Group::Start;
		} else if ( "startCollapsed" == value ) {
			group = Group::StartCollapsed;
		} else if ( "end" == value ) {
			group = Group::End;
		}
	}
	if ( body.contains( DAP_CATEGORY ) ) {
		const auto value = body[DAP_CATEGORY].get<std::string>();
		if ( "console" == value ) {
			category = Category::Console;
		} else if ( "important" == value ) {
			category = Category::Important;
		} else if ( "stdout" == value ) {
			category = Category::Stdout;
		} else if ( "stderr" == value ) {
			category = Category::Stderr;
		} else if ( "telemetry" == value ) {
			category = Category::Telemetry;
		}
	}
}

Output::Output( const std::string& output, const Output::Category& category ) :
	category( category ), output( output ) {}

bool Output::isSpecialOutput() const {
	return ( category != Category::Stderr ) && ( category != Category::Stdout );
}

std::string Source::unifiedId() const {
	return getUnifiedId( path, sourceReference );
}

std::string Source::getUnifiedId( const std::string& path, std::optional<int> sourceReference ) {
	if ( sourceReference.value_or( 0 ) > 0 ) {
		return String::toString( *sourceReference );
	}
	return path;
}

Source::Source( const json& body ) :
	name( body.value( DAP_NAME, "" ) ),
	path( body.value( DAP_PATH, "" ) ),
	sourceReference( parseOptionalInt( body, DAP_SOURCE_REFERENCE ) ),
	presentationHint( parseOptionalString( body, DAP_PRESENTATION_HINT ) ),
	origin( body.value( DAP_ORIGIN, "" ) ),
	adapterData( body.contains( DAP_ADAPTER_DATA ) ? body[DAP_ADAPTER_DATA] : nlohmann::json{} ) {
	// sources
	if ( body.contains( DAP_SOURCES ) ) {
		const auto values = body[DAP_SOURCES];
		for ( const auto& item : values ) {
			sources.emplace_back( Source( item ) );
		}
	}

	// checksums
	if ( body.contains( DAP_CHECKSUMS ) ) {
		const auto values = body[DAP_CHECKSUMS];
		for ( const auto& item : values ) {
			checksums.emplace_back( Checksum( item ) );
		}
	}
}

Source::Source( const std::string& path ) : path( path ) {}

json Source::toJson() const {
	json out;
	if ( !name.empty() ) {
		out[DAP_NAME] = name;
	}
	if ( !path.empty() ) {
		out[DAP_PATH] = path;
	}
	if ( sourceReference ) {
		out[DAP_SOURCE_REFERENCE] = *sourceReference;
	}
	if ( presentationHint ) {
		out[DAP_PRESENTATION_HINT] = *presentationHint;
	}
	if ( !origin.empty() ) {
		out[DAP_ORIGIN] = origin;
	}
	if ( !adapterData.is_null() && !adapterData.empty() ) {
		out[DAP_ADAPTER_DATA] = adapterData;
	}
	if ( !sources.empty() ) {
		out[DAP_SOURCES] = toJsonArray( sources );
	}
	if ( !checksums.empty() ) {
		out[DAP_CHECKSUMS] = toJsonArray( checksums );
	}
	return out;
}

Checksum::Checksum( const json& body ) :
	checksum( body[DAP_CHECKSUM].get<std::string>() ),
	algorithm( body[DAP_ALGORITHM].get<std::string>() ) {}

json Checksum::toJson() const {
	json out;
	out[DAP_CHECKSUM] = checksum;
	out[DAP_ALGORITHM] = algorithm;
	return out;
}

Capabilities::Capabilities( const json& body ) :
	supportsConfigurationDoneRequest( body.value( "supportsConfigurationDoneRequest", false ) ),
	supportsFunctionBreakpoints( body.value( "supportsFunctionBreakpoints", false ) ),
	supportsConditionalBreakpoints( body.value( "supportsConditionalBreakpoints", false ) ),
	supportsHitConditionalBreakpoints( body.value( "supportsHitConditionalBreakpoints", false ) ),
	supportsLogPoints( body.value( "supportsLogPoints", false ) ),
	supportsModulesRequest( body.value( "supportsModulesRequest", false ) ),
	supportsTerminateRequest( body.value( "supportsTerminateRequest", false ) ),
	supportTerminateDebuggee( body.value( "supportTerminateDebuggee", false ) ),
	supportsGotoTargetsRequest( body.value( "supportsGotoTargetsRequest", false ) ) {}

ThreadEvent::ThreadEvent( const json& body ) :
	reason( body[DAP_REASON].get<std::string>() ), threadId( body[DAP_THREAD_ID].get<int>() ) {}

StoppedEvent::StoppedEvent( const json& body ) :
	reason( body[DAP_REASON].get<std::string>() ),
	description( parseOptionalString( body, "description" ) ),
	threadId( body[DAP_THREAD_ID].get<int>() ),
	preserveFocusHint( parseOptionalBool( body, "preserveFocusHint" ) ),
	text( parseOptionalString( body, "text" ) ),
	allThreadsStopped( parseOptionalBool( body, "allThreadsStopped" ) ),
	hitBreakpointsIds( parseOptionalIntList( body, "hitBreakpointsIds" ) ) {}

DapThread::DapThread( const json& body ) :
	id( body[DAP_ID].get<int>() ), name( body[DAP_NAME].get<std::string>() ) {}

DapThread::DapThread( const int id ) : id( id ), name( std::string() ) {}

std::vector<DapThread> DapThread::parseList( const json& threads ) {
	return parseObjectList<DapThread>( threads );
}

StackFrame::StackFrame( const json& body ) :
	id( body[DAP_ID].get<int>() ),
	name( body[DAP_NAME].get<std::string>() ),
	source( parseOptionalObject<Source>( body, DAP_SOURCE ) ),
	line( body[DAP_LINE].get<int>() ),
	column( body[DAP_COLUMN].get<int>() ),
	endLine( parseOptionalInt( body, "endLine" ) ),
	canRestart( parseOptionalBool( body, "canRestart" ) ),
	instructionPointerReference( parseOptionalString( body, "instructionPointerReference" ) ),
	moduleId_int( parseOptionalInt( body, DAP_MODULE_ID ) ),
	moduleId_str( parseOptionalString( body, DAP_MODULE_ID ) ),
	presentationHint( parseOptionalString( body, DAP_PRESENTATION_HINT ) ) {}

StackTraceInfo::StackTraceInfo( const json& body ) :
	stackFrames( parseObjectList<StackFrame>( body["stackFrames"] ) ),
	totalFrames( parseOptionalInt( body, "totalFrames" ) ) {}

Module::Module( const json& body ) :
	id_int( parseOptionalInt( body, DAP_ID ) ),
	id_str( parseOptionalString( body, DAP_ID ) ),
	name( body[DAP_NAME].get<std::string>() ),
	path( parseOptionalString( body, DAP_PATH ) ),
	isOptimized( parseOptionalBool( body, "isOptimized" ) ),
	isUserCode( parseOptionalBool( body, "isUserCode" ) ),
	version( parseOptionalString( body, "version" ) ),
	symbolStatus( parseOptionalString( body, "symbolStatus" ) ),
	symbolFilePath( parseOptionalString( body, "symbolFilePath" ) ),
	dateTimeStamp( parseOptionalString( body, "dateTimeStamp" ) ),
	addressRange( parseOptionalString( body, "addressRange" ) ) {}

ModuleEvent::ModuleEvent( const json& body ) :
	reason( body[DAP_REASON].get<std::string>() ), module( Module( body["module"] ) ) {}

Scope::Scope( const json& body ) :
	name( body[DAP_NAME].get<std::string>() ),
	presentationHint( parseOptionalString( body, DAP_PRESENTATION_HINT ) ),
	variablesReference( body[DAP_VARIABLES_REFERENCE].get<int>() ),
	namedVariables( parseOptionalInt( body, "namedVariables" ) ),
	indexedVariables( parseOptionalInt( body, "indexedVariables" ) ),
	expensive( parseOptionalBool( body, "expensive" ) ),
	source( parseOptionalObject<Source>( body, "source" ) ),
	line( parseOptionalInt( body, "line" ) ),
	column( parseOptionalInt( body, "column" ) ),
	endLine( parseOptionalInt( body, "endLine" ) ),
	endColumn( parseOptionalInt( body, "endColumn" ) ) {}

Scope::Scope( int variablesReference, std::string name ) :
	name( name ), variablesReference( variablesReference ) {}

std::vector<Scope> Scope::parseList( const json& scopes ) {
	return parseObjectList<Scope>( scopes );
}

Variable::Variable( const json& body ) :
	name( body[DAP_NAME].get<std::string>() ),
	value( body["value"].get<std::string>() ),
	type( parseOptionalString( body, DAP_TYPE ) ),
	evaluateName( parseOptionalString( body, "evaluateName" ) ),
	variablesReference( body[DAP_VARIABLES_REFERENCE].get<int>() ),
	namedVariables( parseOptionalInt( body, "namedVariables" ) ),
	indexedVariables( parseOptionalInt( body, "indexedVariables" ) ),
	memoryReference( parseOptionalString( body, "memoryReference" ) ) {}

Variable::Variable( const std::string& name, const std::string& value, const int reference ) :
	name( name ), value( value ), variablesReference( reference ) {}

std::vector<Variable> Variable::parseList( const json& variables ) {
	return parseObjectList<Variable>( variables );
}

ModulesInfo::ModulesInfo( const json& body ) :
	modules( parseObjectList<Module>( body[DAP_MODULES] ) ),
	totalModules( parseOptionalInt( body, "totalModules" ) ) {}

ContinuedEvent::ContinuedEvent( const json& body ) :
	threadId( body[DAP_THREAD_ID].get<int>() ),
	allThreadsContinued( parseOptionalBool( body, DAP_ALL_THREADS_CONTINUED ) ) {}

ContinuedEvent::ContinuedEvent( int threadId, bool allThreadsContinued ) :
	threadId( threadId ), allThreadsContinued( allThreadsContinued ) {}

SourceContent::SourceContent( const json& body ) :
	content( body["content"].get<std::string>() ),
	mimeType( parseOptionalString( body, "mimeType" ) ) {}

SourceContent::SourceContent( const std::string& path ) {
	const FileInfo file( path );
	if ( file.isRegularFile() && file.exists() ) {
		std::string contents;
		FileSystem::fileGet( path, contents );
	}
}

SourceBreakpoint::SourceBreakpoint( const json& body ) :
	line( body[DAP_LINE].get<int>() ),
	column( parseOptionalInt( body, DAP_COLUMN ) ),
	condition( parseOptionalString( body, DAP_CONDITION ) ),
	hitCondition( parseOptionalString( body, DAP_HIT_CONDITION ) ),
	logMessage( parseOptionalString( body, "logMessage" ) ) {}

SourceBreakpoint::SourceBreakpoint( const int line ) : line( line ) {}

json SourceBreakpoint::toJson() const {
	json out;
	out[DAP_LINE] = line;
	if ( condition ) {
		out[DAP_CONDITION] = *condition;
	}
	if ( column ) {
		out[DAP_COLUMN] = *column;
	}
	if ( hitCondition ) {
		out[DAP_HIT_CONDITION] = *hitCondition;
	}
	if ( logMessage ) {
		out[DAP_LOG_MESSAGE] = *logMessage;
	}
	return out;
}

Breakpoint::Breakpoint( const json& body ) :
	id( parseOptionalInt( body, DAP_ID ) ),
	verified( body.value( "verified", false ) ),
	message( parseOptionalString( body, "message" ) ),
	source( parseOptionalObject<Source>( body, DAP_SOURCE ) ),
	line( parseOptionalInt( body, DAP_LINE ) ),
	column( parseOptionalInt( body, DAP_COLUMN ) ),
	endLine( parseOptionalInt( body, DAP_END_LINE ) ),
	endColumn( parseOptionalInt( body, DAP_END_COLUMN ) ),
	instructionReference( parseOptionalString( body, "instructionReference" ) ),
	offset( parseOptionalInt( body, "offset" ) ) {}

Breakpoint::Breakpoint( const int line ) : line( line ) {}

BreakpointEvent::BreakpointEvent( const json& body ) :
	reason( body[DAP_REASON].get<std::string>() ),
	breakpoint( Breakpoint( body[DAP_BREAKPOINT] ) ) {}

EvaluateInfo::EvaluateInfo( const json& body ) :
	result( body[DAP_RESULT].get<std::string>() ),
	type( parseOptionalString( body, DAP_TYPE ) ),
	variablesReference( body[DAP_VARIABLES_REFERENCE].get<int>() ),
	namedVariables( parseOptionalInt( body, "namedVariables" ) ),
	indexedVariables( parseOptionalInt( body, "indexedVariables" ) ),
	memoryReference( parseOptionalString( body, "memoryReference" ) ) {}

GotoTarget::GotoTarget( const json& body ) :
	id( body[DAP_ID].get<int>() ),
	label( body["label"].get<std::string>() ),
	line( body[DAP_LINE].get<int>() ),
	column( parseOptionalInt( body, DAP_COLUMN ) ),
	endLine( parseOptionalInt( body, DAP_END_LINE ) ),
	endColumn( parseOptionalInt( body, DAP_END_COLUMN ) ),
	instructionPointerReference( parseOptionalString( body, "instructionPointerReference" ) ) {}

std::vector<GotoTarget> GotoTarget::parseList( const json& variables ) {
	return parseObjectList<GotoTarget>( variables );
}

} // namespace ecode::dap
