#include "debuggerclientdap.hpp"
#include "messages.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/log.hpp>

using namespace EE::System;

namespace ecode::dap {

constexpr int MAX_HEADER_SIZE = 1 << 16;

template <typename T>
inline DebuggerClientDap::ResponseHandler
makeResponseHandler( void ( T::*member )( const Response& response, const nlohmann::json& request ),
					 T* object ) {
	return [object, member]( const Response& response, const nlohmann::json& request ) {
		return ( object->*member )( response, request );
	};
}

DebuggerClientDap::DebuggerClientDap( const ProtocolSettings& protocolSettings,
									  std::unique_ptr<Bus>&& bus ) :
	mBus( std::move( bus ) ), mProtocol( protocolSettings ) {}

DebuggerClientDap::~DebuggerClientDap() {
	mBus.reset();
}

void DebuggerClientDap::makeRequest( const std::string_view& command,
									 const nlohmann::json& arguments, ResponseHandler onFinish ) {
	nlohmann::json jsonCmd = {
		{ "seq", mIdx.load() },
		{ "type", "request" },
		{ "command", command },
		{ "arguments", arguments.empty() ? nlohmann::json::object() : arguments } };

	std::string cmd = jsonCmd.dump();
	std::string msg( String::format( "Content-Length: %zu\r\n\r\n%s", cmd.size(), cmd ) );

	Log::instance()->writel( mDebug ? LogLevel::Info : LogLevel::Debug,
							 "DebuggerClientDap::makeRequest:" );
	Log::instance()->writel( mDebug ? LogLevel::Info : LogLevel::Debug, msg );

	mBus->write( msg.data(), msg.size() );

	mRequests[mIdx] = { std::string{ command }, arguments, onFinish };

	mIdx.fetch_add( 1, std::memory_order_relaxed );
}

bool DebuggerClientDap::isServerConnected() const {
	return ( mState != State::None ) && ( mState != State::Failed ) &&
		   ( mBus->state() == Bus::State::Running );
}

bool DebuggerClientDap::supportsTerminate() const {
	return mAdapterCapabilities.supportsTerminateRequest &&
		   ( mProtocol.launchRequest.value( DAP_COMMAND, "" ) == DAP_LAUNCH );
}

bool DebuggerClientDap::start() {
	bool started = mBus->start();
	if ( started )
		mBus->startAsyncRead( [this]( const char* bytes, size_t n ) { asyncRead( bytes, n ); } );
	else {
		Log::warning( "DebuggerClientDap::start: could not initialize the debugger" );
		return false;
	}
	mStarted = started;
	mLaunched = false;
	mConfigured = false;
	if ( mState != State::None ) {
		Log::warning( "DebuggerClientDap::start: trying to re-start has no effect" );
		return false;
	}
	requestInitialize();
	return started;
}

void DebuggerClientDap::processResponseInitialize( const Response& response,
												   const nlohmann::json& ) {
	if ( mState != State::Initializing ) {
		Log::warning(
			"DebuggerClientDap::processResponseInitialize: unexpected initialize response" );
		setState( State::None );
		return;
	}

	if ( !response.success && response.isCancelled() ) {
		Log::warning( "DebuggerClientDap::processResponseInitialize: InitializeResponse error: %s",
					  response.message );
		if ( response.errorBody ) {
			Log::warning( "DebuggerClientDap::processResponseInitialize: error %ld %s",
						  response.errorBody->id, response.errorBody->format );
		}
		setState( State::None );
		return;
	}

	// get server capabilities
	mAdapterCapabilities = Capabilities( response.body );
	for ( auto listener : mListeners )
		listener->capabilitiesReceived( mAdapterCapabilities );

	requestLaunchCommand();
}

void DebuggerClientDap::requestLaunchCommand() {
	if ( mState != State::Initializing ) {
		Log::warning(
			"DebuggerClientDap::requestLaunchCommand: trying to launch in an unexpected state" );
		return;
	}

	if ( mProtocol.launchCommand.empty() )
		return;

	makeRequest( mProtocol.launchCommand, mProtocol.launchRequest,
				 [this]( const Response& response, const auto& ) {
					 if ( response.success ) {
						 mLaunched = true;
						 checkRunning();
						 for ( auto listener : mListeners )
							 listener->launched();
					 } else {
						 if ( response.errorBody ) {
							 Log::warning( "DebuggerClientDap::requestLaunchCommand: error %ld %s",
										   response.errorBody->id, response.errorBody->format );
						 }
						 setState( State::Failed );
					 }
				 } );
}

void DebuggerClientDap::requestInitialize() {
	const nlohmann::json capabilities{
		{ DAP_CLIENT_ID, "ecode-dap" },
		{ DAP_CLIENT_NAME, "ecode dap" },
		{ "locale", mProtocol.locale },
		{ DAP_ADAPTER_ID, "ecode-dap" },
		{ DAP_LINES_START_AT1, mProtocol.linesStartAt1 },
		{ DAP_COLUMNS_START_AT2, mProtocol.columnsStartAt1 },
		{ DAP_PATH, ( mProtocol.pathFormatURI ? DAP_URI : DAP_PATH ) },
		{ DAP_SUPPORTS_VARIABLE_TYPE, true },
		{ DAP_SUPPORTS_VARIABLE_PAGING, false },
		{ DAP_SUPPORTS_RUN_IN_TERMINAL_REQUEST, false },
		{ DAP_SUPPORTS_MEMORY_REFERENCES, false },
		{ DAP_SUPPORTS_PROGRESS_REPORTING, false },
		{ DAP_SUPPORTS_INVALIDATED_EVENT, false },
		{ DAP_SUPPORTS_MEMORY_EVENT, false } };

	setState( State::Initializing );
	makeRequest( DAP_INITIALIZE, capabilities,
				 makeResponseHandler( &DebuggerClientDap::processResponseInitialize, this ) );
}

void DebuggerClientDap::asyncRead( const char* bytes, size_t n ) {
	std::string_view read( bytes, n );
	mBuffer += read;

	while ( true ) {
		const auto info = readHeader();
		if ( !info )
			break;
		const auto data = mBuffer.substr( info->payloadStart, info->payloadLength );
		if ( data.size() < info->payloadLength )
			break;
		mBuffer.erase( 0, info->payloadStart + info->payloadLength );

#ifndef EE_DEBUG
		try {
#endif
			auto message = json::parse( data );

			if ( mDebug ) {
				Log::debug( "DebuggerClientDap::asyncRead:" );
				Log::debug( message.dump() );
			}

			processProtocolMessage( message );
#ifndef EE_DEBUG
		} catch ( const json::exception& e ) {
			Log::error( "DebuggerClientDap::asyncRead: JSON bad format: %s", e.what() );
		}
#endif
	}
}

void DebuggerClientDap::processProtocolMessage( const nlohmann::json& msg ) {
	const auto type = msg.value( DAP_TYPE, "" );

	if ( DAP_RESPONSE == type ) {
		processResponse( msg );
	} else if ( DAP_EVENT == type ) {
		processEvent( msg );
	} else {
		Log::warning( "DebuggerClientDap::processProtocolMessage: unknown, empty or unexpected "
					  "ProtocolMessage::%s (%s)",
					  DAP_TYPE, type );
	}
}

void DebuggerClientDap::processResponse( const nlohmann::json& msg ) {
	const Response response( msg );

	// check sequence
	if ( ( response.request_seq < 0 ) || 0 == mRequests.count( response.request_seq ) ) {
		Log::error( "DebuggerClientDap::processResponse: unexpected requested seq in response" );
		return;
	}

	const auto request = mRequests.extract( response.request_seq ).mapped();

	// check response
	if ( response.command != request.command ) {
		Log::error(
			"DebuggerClientDap::processResponse: unexpected command in response: %s (expected: %s)",
			response.command, request.command );
	}

	if ( response.isCancelled() )
		Log::debug( "DebuggerClientDap::processResponse: request cancelled: %s", response.command );

	if ( !response.success )
		return errorResponse( response.message, response.errorBody );

	if ( request.handler ) {
		request.handler( response, request.arguments );
	}
}

void DebuggerClientDap::errorResponse( const std::string& summary,
									   const std::optional<Message>& message ) {
	for ( auto listener : mListeners )
		listener->errorResponse( summary, message );
}

void DebuggerClientDap::processEvent( const nlohmann::json& msg ) {
	const std::string event = msg.value( DAP_EVENT, "" );
	const auto body = msg.contains( DAP_BODY ) ? msg[DAP_BODY] : nlohmann::json{};

	if ( "initialized"sv == event ) {
		processEventInitialized();
	} else if ( "terminated"sv == event ) {
		processEventTerminated();
	} else if ( "exited"sv == event ) {
		processEventExited( body );
	} else if ( DAP_OUTPUT == event ) {
		processEventOutput( body );
	} else if ( "process"sv == event ) {
		processEventProcess( body );
	} else if ( "thread"sv == event ) {
		processEventThread( body );
	} else if ( "stopped"sv == event ) {
		processEventStopped( body );
	} else if ( "module"sv == event ) {
		processEventModule( body );
	} else if ( "continued"sv == event ) {
		processEventContinued( body );
	} else if ( DAP_BREAKPOINT == event ) {
		processEventBreakpoint( body );
	} else {
		Log::info( "DebuggerClientDap::processEvent: unsupported event: %s", event );
	}
}

void DebuggerClientDap::processEventInitialized() {
	if ( ( mState != State::Initializing ) ) {
		Log::error( "DebuggerClientDap::processEventInitialized: unexpected initialized event" );
		return;
	}
	setState( State::Initialized );

	configurationDone();
}

void DebuggerClientDap::processEventTerminated() {
	setState( State::Terminated );
}

void DebuggerClientDap::processEventExited( const nlohmann::json& body ) {
	const int exitCode = body.value( "exitCode", -1 );
	for ( auto listener : mListeners )
		listener->debuggeeExited( exitCode );
}

void DebuggerClientDap::processEventOutput( const nlohmann::json& body ) {
	Output output( body );
	for ( auto listener : mListeners )
		listener->outputProduced( output );
}

void DebuggerClientDap::processEventProcess( const nlohmann::json& body ) {
	ProcessInfo processInfo( body );
	for ( auto listener : mListeners )
		listener->debuggingProcess( processInfo );
}

void DebuggerClientDap::processEventThread( const nlohmann::json& body ) {
	ThreadEvent threadEvent( body );
	for ( auto listener : mListeners )
		listener->threadChanged( threadEvent );
}

void DebuggerClientDap::processEventStopped( const nlohmann::json& body ) {
	StoppedEvent stoppedEvent( body );
	for ( auto listener : mListeners )
		listener->debuggeeStopped( stoppedEvent );
}

void DebuggerClientDap::processEventModule( const nlohmann::json& body ) {
	ModuleEvent moduleEvent( body );
	for ( auto listener : mListeners )
		listener->moduleChanged( moduleEvent );
}

void DebuggerClientDap::processEventContinued( const nlohmann::json& body ) {
	ContinuedEvent continuedEvent( body );
	for ( auto listener : mListeners )
		listener->debuggeeContinued( continuedEvent );
}

void DebuggerClientDap::processEventBreakpoint( const nlohmann::json& body ) {
	BreakpointEvent breakpointEvent( body );
	for ( auto listener : mListeners )
		listener->breakpointChanged( breakpointEvent );
}

std::optional<DebuggerClientDap::HeaderInfo> DebuggerClientDap::readHeader() {
	Uint64 length = std::string::npos;
	Uint64 start = 0;
	Uint64 end = std::string::npos;

	auto discardExploredBuffer = [this, length, start, end]() mutable {
		mBuffer.erase( 0, end );
		length = end = std::string::npos;
		start = 0;
	};

	while ( true ) {
		end = mBuffer.find( DAP_SEP, start );
		if ( end == std::string::npos ) {
			if ( mBuffer.size() > MAX_HEADER_SIZE )
				mBuffer.clear();
			length = std::string::npos;
			break; // PENDING
		}

		const auto header = mBuffer.substr( start, end - start );
		while ( std::string_view{ mBuffer }.substr( end, 2 ) == DAP_SEP )
			end += DAP_SEP_SIZE;

		// header block separator
		if ( header.size() == 0 ) {
			if ( length < 0 ) {
				// unexpected end of header
				Log::error( "DebuggerClientDap::readHeader unexpected end of header block" );
				discardExploredBuffer();
				continue;
			}
			break; // END HEADER (length>0, end>0)
		}

		// parse field
		const auto sep = header.find_first_of( ":" );
		if ( sep == std::string::npos ) {
			Log::error( "DebuggerClientDap::readHeader cannot parse header field: ", header );
			discardExploredBuffer();
			continue; // CONTINUE HEADER
		}

		// parse content-length
		if ( String::startsWith( header, DAP_CONTENT_LENGTH ) ) {
			std::string lengthStr( header.substr( sep + 1, header.size() - sep ) );
			String::trimInPlace( lengthStr );
			if ( !String::fromString( length, lengthStr ) ) {
				Log::error( "DebuggerClientDap::readHeader invalid value: ", header );
				discardExploredBuffer();
				continue; // CONTINUE HEADER
			} else {
				break;
			}
		}

		start = end;
	}

	if ( length < 0 || length == std::string::npos )
		return std::nullopt;

	return HeaderInfo{ end, length };
}

bool DebuggerClientDap::resume( int threadId, bool singleThread ) {
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest( "continue", arguments,
				 [this]( const Response& response, const nlohmann::json& request ) {
					 if ( response.success ) {
						 ContinuedEvent continuedEvent(
							 request.value( DAP_THREAD_ID, 1 ),
							 response.body.value( DAP_ALL_THREADS_CONTINUED, true ) );
						 for ( auto listener : mListeners )
							 listener->debuggeeContinued( continuedEvent );
					 }
				 } );
	return true;
}

bool DebuggerClientDap::pause( int threadId ) {
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	makeRequest( "pause", arguments );
	return true;
}

void DebuggerClientDap::processResponseNext( const Response& response,
											 const nlohmann::json& request ) {
	if ( response.success ) {
		bool all = false;
		if ( response.body.is_object() && response.body.contains( DAP_ALL_THREADS_CONTINUED ) )
			all = response.body.value( DAP_ALL_THREADS_CONTINUED, false );

		ContinuedEvent continuedEvent( request.value( DAP_THREAD_ID, 1 ), all );
		for ( auto listener : mListeners )
			listener->debuggeeContinued( continuedEvent );
	}
}

bool DebuggerClientDap::stepOver( int threadId, bool singleThread ) {
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest( "next", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this ) );
	return true;
}

bool DebuggerClientDap::goTo( int threadId, int targetId ) {
	const nlohmann::json arguments{ { DAP_THREAD_ID, threadId }, { DAP_TARGET_ID, targetId } };
	makeRequest( "goto", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this ) );
	return true;
}

bool DebuggerClientDap::stepInto( int threadId, bool singleThread ) {
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest( "stepIn", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this ) );
	return true;
}

bool DebuggerClientDap::stepOut( int threadId, bool singleThread ) {
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest( "stepOut", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this ) );
	return true;
}

bool DebuggerClientDap::terminate( bool restart ) {
	nlohmann::json arguments;
	if ( restart )
		arguments["restart"] = true;
	makeRequest( "terminate", arguments );
	return true;
}

bool DebuggerClientDap::disconnect( bool restart ) {
	nlohmann::json arguments;
	if ( restart )
		arguments["restart"] = true;

	makeRequest( "disconnect", arguments,
				 [this]( const Response& response, const nlohmann::json& ) {
					 if ( response.success ) {
						 for ( auto listener : mListeners )
							 listener->serverDisconnected();
					 }
				 } );
	return true;
}

bool DebuggerClientDap::threads() {
	makeRequest( DAP_THREADS, {}, [this]( const Response& response, const nlohmann::json& ) {
		if ( response.success ) {
			std::vector<DapThread> threads( DapThread::parseList( response.body[DAP_THREADS] ) );
			for ( auto listener : mListeners )
				listener->threads( std::move( threads ) );
		} else {
			for ( auto listener : mListeners )
				listener->threads( {} );
		}
	} );
	return true;
}

bool DebuggerClientDap::stackTrace( int threadId, int startFrame, int levels ) {
	const nlohmann::json arguments{
		{ DAP_THREAD_ID, threadId }, { "startFrame", startFrame }, { "levels", levels } };

	makeRequest( "stackTrace", arguments,
				 [this]( const Response& response, const nlohmann::json& request ) {
					 const int threadId = request.value( DAP_THREAD_ID, 1 );
					 if ( response.success ) {
						 StackTraceInfo stackTraceInfo( response.body );
						 for ( auto listener : mListeners )
							 listener->stackTrace( threadId, std::move( stackTraceInfo ) );
					 } else {
						 StackTraceInfo stackTraceInfo;
						 for ( auto listener : mListeners )
							 listener->stackTrace( threadId, std::move( stackTraceInfo ) );
					 }
				 } );
	return true;
}

bool DebuggerClientDap::scopes( int frameId ) {
	const nlohmann::json arguments{ { DAP_FRAME_ID, frameId } };
	makeRequest( DAP_SCOPES, arguments,
				 [this]( const Response& response, const nlohmann::json& request ) {
					 const int frameId = request.value( DAP_FRAME_ID, 1 );
					 if ( response.success ) {
						 auto scopes( Scope::parseList( response.body[DAP_SCOPES] ) );
						 for ( auto listener : mListeners )
							 listener->scopes( frameId, std::move( scopes ) );
					 } else {
						 std::vector<Scope> scopes;
						 for ( auto listener : mListeners )
							 listener->scopes( frameId, std::move( scopes ) );
					 }
				 } );
	return true;
}

bool DebuggerClientDap::variables( int variablesReference, Variable::Type filter, int start,
								   int count ) {
	nlohmann::json arguments{
		{ DAP_VARIABLES_REFERENCE, variablesReference },
		{ DAP_START, start },
		{ DAP_COUNT, count },
	};

	switch ( filter ) {
		case Variable::Type::Indexed:
			arguments[DAP_FILTER] = "indexed";
			break;
		case Variable::Type::Named:
			arguments[DAP_FILTER] = "named";
			break;
		default:
			break;
	}

	makeRequest( DAP_VARIABLES, arguments,
				 [this]( const Response& response, const nlohmann::json& request ) {
					 const int variablesReference = request.value( DAP_VARIABLES_REFERENCE, 0 );

					 if ( response.success ) {
						 auto variableList( Variable::parseList( response.body[DAP_VARIABLES] ) );
						 for ( auto listener : mListeners )
							 listener->variables( variablesReference, std::move( variableList ) );
					 } else {
						 std::vector<Variable> variableList;
						 for ( auto listener : mListeners )
							 listener->variables( variablesReference, std::move( variableList ) );
					 }
				 } );

	return true;
}

bool DebuggerClientDap::modules( int start, int count ) {
	makeRequest( DAP_MODULES, { { DAP_START, start }, { DAP_COUNT, count } },
				 [this]( const auto& response, const auto& ) {
					 if ( response.success ) {
						 ModulesInfo info( response.body );
						 for ( auto listener : mListeners )
							 listener->modules( std::move( info ) );
					 } else {
						 ModulesInfo info;
						 for ( auto listener : mListeners )
							 listener->modules( std::move( info ) );
					 }
				 } );
	return true;
}

bool DebuggerClientDap::evaluate( const std::string& expression, const std::string& context,
								  std::optional<int> frameId ) {
	nlohmann::json arguments{ { DAP_EXPRESSION, expression } };
	if ( !context.empty() )
		arguments[DAP_CONTEXT] = context;
	if ( frameId )
		arguments[DAP_FRAME_ID] = *frameId;

	makeRequest( "evaluate", arguments, [this]( const auto& response, const auto& request ) {
		auto expression = request.value( DAP_EXPRESSION, "" );
		if ( response.success ) {
			EvaluateInfo info( response.body );

			for ( auto listener : mListeners )
				listener->expressionEvaluated( expression, info );
		} else {
			for ( auto listener : mListeners )
				listener->expressionEvaluated( expression, std::nullopt );
		}
	} );

	return true;
}

bool DebuggerClientDap::setBreakpoints( const std::string& path,
										const std::vector<dap::SourceBreakpoint>& breakpoints,
										bool sourceModified ) {
	return setBreakpoints( Source( path ), breakpoints, sourceModified );
}

bool DebuggerClientDap::setBreakpoints( const dap::Source& source,
										const std::vector<dap::SourceBreakpoint>& breakpoints,
										bool sourceModified ) {
	nlohmann::json bpoints = nlohmann::json::array();
	for ( const auto& item : breakpoints )
		bpoints.push_back( item.toJson() );

	nlohmann::json arguments{ { DAP_SOURCE, source.toJson() },
							  { DAP_BREAKPOINTS, bpoints },
							  { "sourceModified", sourceModified } };

	makeRequest( "setBreakpoints", arguments,
				 [this]( const Response& response, const nlohmann::json& request ) {
					 const auto source = Source( request[DAP_SOURCE] );
					 if ( response.success ) {
						 const auto resp = response.body;
						 if ( resp.contains( DAP_BREAKPOINTS ) ) {
							 std::vector<Breakpoint> breakpoints;
							 breakpoints.reserve( resp[DAP_BREAKPOINTS].size() );
							 for ( const auto& item : resp[DAP_BREAKPOINTS] )
								 breakpoints.emplace_back( item );

							 for ( auto listener : mListeners )
								 listener->sourceBreakpoints( source.path,
															  source.sourceReference.value_or( 0 ),
															  breakpoints );
						 } else {
							 std::vector<Breakpoint> breakpoints;
							 breakpoints.reserve( resp[DAP_LINES].size() );
							 for ( const auto& item : resp[DAP_LINES] )
								 breakpoints.emplace_back( item.get<int>() );

							 for ( auto listener : mListeners )
								 listener->sourceBreakpoints( source.path,
															  source.sourceReference.value_or( 0 ),
															  breakpoints );
						 }
					 } else {
						 for ( auto listener : mListeners )
							 listener->sourceBreakpoints(
								 source.path, source.sourceReference.value_or( 0 ), std::nullopt );
					 }
				 } );

	return true;
}

bool DebuggerClientDap::gotoTargets( const std::string& path, const int line,
									 const std::optional<int> column ) {
	return gotoTargets( Source( path ), line, column );
}

bool DebuggerClientDap::gotoTargets( const Source& source, const int line,
									 const std::optional<int> column ) {
	nlohmann::json arguments{ { DAP_SOURCE, source.toJson() }, { DAP_LINE, line } };
	if ( column )
		arguments[DAP_COLUMN] = *column;

	makeRequest( "gotoTargets", arguments, [this]( const auto& response, const auto& req ) {
		const auto source = Source( req[DAP_SOURCE] );
		const int line = req.value( DAP_LINE, 1 );
		if ( response.success ) {
			auto list = GotoTarget::parseList( response.body["targets"] );
			for ( auto listener : mListeners )
				listener->gotoTargets( source, line, list );
		} else {
			std::vector<GotoTarget> list;
			for ( auto listener : mListeners )
				listener->gotoTargets( source, line, list );
		}
	} );

	return true;
}

bool DebuggerClientDap::watch( const std::string& expression, std::optional<int> frameId ) {
	return evaluate( expression, "watch", frameId );
}

bool DebuggerClientDap::configurationDone() {
	if ( mState != State::Initialized ) {
		Log::warning( "DebuggerClientDap::requestConfigurationDone: trying to configure in an "
					  "unexpected status" );
		return false;
	}

	if ( !mAdapterCapabilities.supportsConfigurationDoneRequest ) {
		for ( auto listener : mListeners )
			listener->configured();
		return true;
	}

	makeRequest( "configurationDone", nlohmann::json{},
				 [this]( const auto& response, const auto& ) {
					 if ( response.success ) {
						 mConfigured = true;
						 checkRunning();
						 for ( auto listener : mListeners )
							 listener->configured();
					 }
				 } );

	return true;
}

bool DebuggerClientDap::started() const {
	return mStarted;
}

} // namespace ecode::dap
