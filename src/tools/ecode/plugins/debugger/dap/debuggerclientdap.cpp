#include "debuggerclientdap.hpp"
#include "../busprocess.hpp"
#include "../bussocket.hpp"
#include "../bussocketprocess.hpp"
#include "messages.hpp"
#include <cinttypes>
#include <eepp/core/string.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/uuid.hpp>

using namespace EE::System;

namespace ecode::dap {

constexpr int MAX_HEADER_SIZE = 1 << 16;

template <typename T>
inline DebuggerClientDap::ResponseHandler makeResponseHandler(
	void ( T::*member )( const Response&, const nlohmann::json&, const std::string& ), T* object,
	const SessionId& sessionId ) {
	return [object, member, sessionId]( const Response& response, const nlohmann::json& request ) {
		( object->*member )( response, request, sessionId );
	};
}

DebuggerClientDap::DebuggerClientDap( const ProtocolSettings& protocolSettings,
									  std::unique_ptr<Bus>&& bus ) :
	mProtocol( protocolSettings ) {
	mCurrentSessionId = "main";
	Lock l( mSessionsMutex );
	mSessions[mCurrentSessionId].bus = std::move( bus );
}

DebuggerClientDap::~DebuggerClientDap() {
	mDestroying = true;
	mBusesToClose.clear();

	{
		Lock l( mSessionsMutex );
		for ( auto& s : mSessions )
			if ( s.second.bus )
				s.second.bus.reset();
	}
}

void DebuggerClientDap::makeRequest( const std::string_view& command,
									 const nlohmann::json& arguments, ResponseHandler onFinish,
									 const SessionId& sessionId ) {
	auto& session = mSessions[sessionId];

	nlohmann::json jsonCmd = {
		{ "seq", session.idx.load() },
		{ "type", "request" },
		{ "command", command },
		{ "arguments", arguments.empty() ? nlohmann::json::object() : arguments } };

	std::string cmd = jsonCmd.dump();
	std::string msg( String::format( "Content-Length: %zu\r\n\r\n%s", cmd.size(), cmd ) );

	if ( mDebug ) {
		Log::debug( "DebuggerClientDap::makeRequest [Session %s]:", sessionId );
		Log::debug( msg );
	}

	if ( !session.bus )
		return;

	session.bus->write( msg.data(), msg.size() );

	Lock l( mRequestsMutex );
	mRequests[session.idx] = { std::string{ command }, arguments, onFinish, sessionId };
	session.idx.fetch_add( 1, std::memory_order_relaxed );
}

void DebuggerClientDap::makeResponse( int reqSeq, bool success, const std::string& command,
									  const nlohmann::json& body, const std::string& sessionId ) {
	auto& session = mSessions[sessionId];
	session.idx.fetch_add( 1, std::memory_order_relaxed );

	nlohmann::json jsonCmd = { { "type", "response" },
							   { "request_seq", reqSeq },
							   { "seq", session.idx.load() },
							   { "success", success },
							   { "command", command } };

	if ( !body.empty() )
		jsonCmd["body"] = body;

	std::string cmd = jsonCmd.dump();
	std::string msg( String::format( "Content-Length: %zu\r\n\r\n%s", cmd.size(), cmd ) );

	if ( mDebug ) {
		Log::debug( "DebuggerClientDap::makeResponse:" );
		Log::debug( msg );
	}

	if ( !mSessions[sessionId].bus )
		return;

	mSessions[sessionId].bus->write( msg.data(), msg.size() );
}

bool DebuggerClientDap::isServerConnected() const {
	if ( !mSessions.empty() ) {
		auto sessionIt = mSessions.find( mCurrentSessionId );
		if ( sessionIt != mSessions.end() && sessionIt->second.bus )
			return sessionIt->second.bus->state() == Bus::State::Running;
	}
	return false;
}

bool DebuggerClientDap::supportsTerminateRequest() const {
	return mAdapterCapabilities.supportsTerminateRequest;
}

bool DebuggerClientDap::supportsTerminateDebuggee() const {
	return mAdapterCapabilities.supportTerminateDebuggee;
}

bool DebuggerClientDap::start() {
	bool started = false;
	if ( mSessions[mCurrentSessionId].bus &&
		 ( started = mSessions[mCurrentSessionId].bus->start() ) ) {
		mSessions[mCurrentSessionId].bus->startAsyncRead(
			[this]( const char* bytes, size_t n ) { asyncRead( bytes, n ); } );
	} else {
		Log::warning( "DebuggerClientDap::start: could not initialize the debugger" );
		return false;
	}
	mStarted = started;
	requestInitialize( mCurrentSessionId );
	return started;
}

void DebuggerClientDap::processResponseInitialize( const Response& response, const nlohmann::json&,
												   const SessionId& sessionId,
												   std::function<void()> onLaunch ) {
	Session& session = mSessions[sessionId];
	if ( session.state != State::Initializing ) {
		Log::warning( "DebuggerClientDap::processResponseInitialize [Session %s]: unexpected "
					  "initialize response",
					  sessionId );
		setState( State::None, sessionId );
		return;
	}

	if ( !response.success ) {
		Log::warning( "DebuggerClientDap::processResponseInitialize [Session %s]: error: %s",
					  sessionId, response.message );
		if ( response.errorBody ) {
			Log::warning( "DebuggerClientDap::processResponseInitialize [Session %s]: error %ld %s",
						  sessionId, response.errorBody->id, response.errorBody->format );
		}
		setState( State::None, sessionId );
		return;
	}

	// get server capabilities
	mAdapterCapabilities = Capabilities( response.body );
	for ( auto listener : mListeners )
		listener->capabilitiesReceived( mAdapterCapabilities );

	requestLaunchCommand( sessionId, onLaunch );
}

void DebuggerClientDap::requestLaunchCommand( const SessionId& sessionId,
											  std::function<void()> onLaunch ) {
	Session& session = mSessions[sessionId];
	if ( session.state != State::Initializing ) {
		Log::warning( "DebuggerClientDap::requestLaunchCommand [Session %s]: unexpected state",
					  sessionId );
		return;
	}

	if ( session.launchRequestType.empty() )
		session.launchRequestType = mProtocol.launchRequestType;
	if ( session.launchArgs.empty() )
		session.launchArgs = mProtocol.launchArgs;

	if ( session.launchRequestType.empty() )
		return;

	makeRequest(
		session.launchRequestType, session.launchArgs,
		[this, sessionId, onLaunch = std::move( onLaunch )]( const Response& response,
															 const auto& ) {
			Session& session = mSessions[sessionId];
			if ( response.success ) {
				session.launched = true;
				checkRunning( sessionId );
				for ( auto listener : mListeners )
					listener->launched( sessionId );
			} else {
				if ( response.errorBody ) {
					Log::warning(
						"DebuggerClientDap::requestLaunchCommand [Session %s]: error %ld %s",
						sessionId, response.errorBody->id, response.errorBody->format );
				}
				setState( State::Failed, sessionId );
			}
			if ( onLaunch )
				onLaunch();
		},
		sessionId );

	if ( mProtocol.runTarget && runTargetCb ) {
		if ( !session.launchArgs.contains( "waitFor" ) )
			runTargetCb();
		else
			session.waitingToAttach = true;
	}
}

void DebuggerClientDap::requestInitialize( const SessionId& sessionId,
										   std::function<void()> onLaunch ) {
	const nlohmann::json capabilities{
		{ DAP_CLIENT_ID, "ecode" },
		{ DAP_CLIENT_NAME, "ecode" },
		{ DAP_LOCALE, mProtocol.locale },
		{ DAP_ADAPTER_ID, "ecode-dap" },
		{ DAP_LINES_START_AT1, mProtocol.linesStartAt1 },
		{ DAP_COLUMNS_START_AT2, mProtocol.columnsStartAt1 },
		{ DAP_PATH, ( mProtocol.pathFormatURI ? DAP_URI : DAP_PATH ) },
		{ DAP_SUPPORTS_VARIABLE_TYPE, true },
		{ DAP_SUPPORTS_VARIABLE_PAGING, false },
		{ DAP_SUPPORTS_RUN_IN_TERMINAL_REQUEST, true },
		{ DAP_SUPPORTS_MEMORY_REFERENCES, true },
		{ DAP_SUPPORTS_PROGRESS_REPORTING, false },
		{ DAP_SUPPORTS_INVALIDATED_EVENT, false },
		{ DAP_SUPPORTS_MEMORY_EVENT, false } };

	setState( State::Initializing, sessionId );
	makeRequest(
		DAP_INITIALIZE, capabilities,
		[this, sessionId, onLaunch = std::move( onLaunch )]( const Response& response,
															 const nlohmann::json& request ) {
			processResponseInitialize( response, request, sessionId, onLaunch );
		},
		sessionId );
}

void DebuggerClientDap::asyncRead( const char* bytes, size_t n ) {
	std::string_view read( bytes, n );
	mBuffer += read;

	while ( true ) {
		const auto info = readHeader();
		if ( !info )
			break;
		if ( info->payloadStart >= mBuffer.size() ) {
			Log::debug( "DebuggerClientDap::asyncRead: Out of range payloadStart access. Current "
						"buffer was:\n%s\nPayload started at %" PRIu64 "",
						mBuffer.data(), info->payloadStart );
			break;
		}
		const auto data = mBuffer.substr( info->payloadStart, info->payloadLength );
		if ( data.size() < info->payloadLength )
			break;
		mBuffer.erase( 0, info->payloadStart + info->payloadLength );

#ifndef EE_DEBUG
		try {
#endif
			auto message = nlohmann::json::parse( data );
			if ( mDebug ) {
				Log::debug( "DebuggerClientDap::asyncRead:" );
				Log::debug( message.dump() );
			}
			processProtocolMessage( message );
#ifndef EE_DEBUG
		} catch ( const nlohmann::json::exception& e ) {
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
	} else if ( DAP_REQUEST == type ) {
		processRequest( msg );
	} else {
		Log::warning( "DebuggerClientDap::processProtocolMessage: unknown type: %s", type );
	}
}

void DebuggerClientDap::processRequest( const nlohmann::json& msg ) {
	const auto reqSeq = msg.value( DAP_SEQ, 0 );
	if ( reqSeq == 0 )
		return;
	const auto command = msg.value( DAP_COMMAND, "" );
	const auto args = msg.contains( DAP_ARGUMENTS ) ? msg[DAP_ARGUMENTS] : nlohmann::json{};

	if ( DAP_RUN_IN_TERMINAL == command ) {
		if ( !runInTerminalCb )
			return;
		bool isIntegrated = args.value( "kind", "integrated" ) == "integrated";
		std::vector<std::string> largs;
		if ( args.contains( "args" ) && args["args"].is_array() ) {
			for ( const auto& jarg : args["args"] ) {
				if ( jarg.is_string() )
					largs.emplace_back( jarg.get<std::string>() );
			}
		}
		std::unordered_map<std::string, std::string> lenv;
		if ( args.contains( "env" ) && args["env"].is_object() ) {
			for ( auto& [key, value] : args["env"].items() ) {
				if ( value.is_string() )
					lenv.emplace( key, value.get<std::string>() );
			}
		}
		std::string cwd = args.value( "cwd", "" );
		if ( !largs.empty() ) {
			std::string cmd = std::move( largs.front() );
			largs.erase( largs.begin() );
			runInTerminalCb(
				isIntegrated, cmd, largs, cwd, lenv, [this, reqSeq, command]( int pid ) {
					makeResponse( reqSeq, pid != 0, command, nlohmann::json{ { "processId", pid } },
								  mCurrentSessionId );
				} );
		}
	} else if ( DAP_START_DEBUGGING == command ) {
		std::string sessionId =
			args.contains( "configuration" )
				? args["configuration"].value( "__pendingTargetId",
											   "" ) // This is non-standard, we recover the target
													// id for node, otherwise we will use an UUID
				: "";
		if ( sessionId.empty() )
			sessionId = UUID().toString();

		Lock l( mSessionsMutex );
		Session& newSession = mSessions[sessionId];
		newSession.launchRequestType = msg.value( DAP_REQUEST, "launch" );

		if ( args.contains( "configuration" ) )
			newSession.launchArgs = args["configuration"];

		newSession.state = State::Initializing;

		switch ( mSessions[mCurrentSessionId].bus->type() ) {
			case Bus::Type::Process: {
				BusProcess* prevBus =
					static_cast<BusProcess*>( mSessions[mCurrentSessionId].bus.get() );
				newSession.bus = std::make_unique<BusProcess>( prevBus->getCommand() );
				break;
			}
			case Bus::Type::Socket: {
				BusSocket* prevBus =
					static_cast<BusSocket*>( mSessions[mCurrentSessionId].bus.get() );
				newSession.bus = std::make_unique<BusSocket>( prevBus->getConnection() );
				break;
			}
			case Bus::Type::SocketProcess: {
				BusSocketProcess* prevBus =
					static_cast<BusSocketProcess*>( mSessions[mCurrentSessionId].bus.get() );
				newSession.bus = std::make_unique<BusSocket>( prevBus->getConnection() );
				break;
			}
		}

		auto prevSessionId = mCurrentSessionId;
		mCurrentSessionId = sessionId;

		if ( newSession.bus && newSession.bus->start() ) {
			newSession.bus->startAsyncRead(
				[this]( const char* bytes, size_t n ) { asyncRead( bytes, n ); } );
		} else {
			Log::warning( "DebuggerClientDap::start: could not initialize the debugger" );
			return;
		}

		requestInitialize( sessionId, [reqSeq, this, prevSessionId] {
			makeResponse( reqSeq, true, DAP_START_DEBUGGING, {}, prevSessionId );
		} );
	}
}

void DebuggerClientDap::processResponse( const nlohmann::json& msg ) {
	const Response response( msg );
	Request request;
	bool foundRequest = false;

	{
		Lock lock( mRequestsMutex );
		auto it = mRequests.find( response.request_seq );
		if ( it == mRequests.end() ) {
			Log::error( "DebuggerClientDap::processResponse: unexpected request seq" );
			return;
		}

		request = std::move( it->second );
		mRequests.erase( it );
		foundRequest = true;
	}

	if ( !foundRequest )
		return;

	if ( response.command != request.command ) {
		Log::error( "DebuggerClientDap::processResponse: command mismatch: %s (expected: %s)",
					response.command, request.command );
	}

	if ( response.isCancelled() )
		Log::debug( "DebuggerClientDap::processResponse: request cancelled: %s", response.command );

	if ( !response.success ) {
		errorResponse( response.command, response.message, response.errorBody, request.sessionId );
		return;
	}

	if ( request.handler )
		request.handler( response, request.arguments );
}

void DebuggerClientDap::errorResponse( const std::string& command, const std::string& summary,
									   const std::optional<Message>& message,
									   const SessionId& sessionId ) {
	for ( auto listener : mListeners )
		listener->errorResponse( command, summary, message, sessionId );
}

void DebuggerClientDap::processEvent( const nlohmann::json& msg ) {
	const std::string event = msg.value( DAP_EVENT, "" );
	const auto body = msg.contains( DAP_BODY ) ? msg[DAP_BODY] : nlohmann::json{};

	// For events tied to threads, find the session
	std::string sessionId;
	if ( body.contains( "threadId" ) ) {
		int threadId = body["threadId"].get<int>();
		sessionId = findSessionByThread( threadId );
	} else {
		// Default to current session or first initializing session
		for ( const auto& [id, session] : mSessions ) {
			if ( session.state == State::Initializing || id == mCurrentSessionId ) {
				sessionId = id;
				break;
			}
		}
	}

	if ( sessionId.empty() ) {
		Log::warning( "DebuggerClientDap::processEvent: no session found for event %s", event );
		return;
	}

	if ( "initialized" == event ) {
		processEventInitialized( sessionId );
	} else if ( "terminated" == event ) {
		processEventTerminated( sessionId );
	} else if ( "exited" == event ) {
		processEventExited( body, sessionId );
	} else if ( DAP_OUTPUT == event ) {
		processEventOutput( body, sessionId );
	} else if ( "process" == event ) {
		processEventProcess( body, sessionId );
	} else if ( "thread" == event ) {
		processEventThread( body, sessionId );
	} else if ( "stopped" == event ) {
		processEventStopped( body, sessionId );
	} else if ( "module" == event ) {
		processEventModule( body, sessionId );
	} else if ( "continued" == event ) {
		processEventContinued( body, sessionId );
	} else if ( DAP_BREAKPOINT == event ) {
		processEventBreakpoint( body, sessionId );
	} else if ( "loadedSource" == event ) {
		// We don't care
	} else {
		Log::info( "DebuggerClientDap::processEvent: unsupported event: %s", event );
	}
}

void DebuggerClientDap::processEventInitialized( const SessionId& sessionId ) {
	Session& session = mSessions[sessionId];
	if ( session.state != State::Initializing ) {
		Log::error( "DebuggerClientDap::processEventInitialized [Session %s]: unexpected event",
					sessionId );
		return;
	}
	setState( State::Initialized, sessionId );
	configurationDone( sessionId );
}

void DebuggerClientDap::processEventTerminated( const SessionId& sessionId ) {
	setState( State::Terminated, sessionId );

	if ( !mDestroying ) {
		Lock l( mSessionsMutex );

		// Clean up terminated session
		auto session = mSessions.extract( sessionId );

		// We cannot destroy the Bus yet, since the async read from the socket might have emited
		// this termination.
		if ( session.mapped().bus ) {
			session.mapped().bus->close();
			mBusesToClose.emplace_back( std::move( session.mapped().bus ) );
		}

		if ( sessionId == mCurrentSessionId && "main" != sessionId )
			mCurrentSessionId = "main";
	}

	for ( auto listener : mListeners )
		listener->debuggeeTerminated( sessionId );
}

void DebuggerClientDap::processEventExited( const nlohmann::json& body,
											const SessionId& sessionId ) {
	const int exitCode = body.value( "exitCode", -1 );

	for ( auto listener : mListeners )
		listener->debuggeeExited( exitCode, sessionId );
}

void DebuggerClientDap::processEventOutput( const nlohmann::json& body,
											const SessionId& sessionId ) {
	Output output( body );
	for ( auto listener : mListeners )
		listener->outputProduced( output );

	Session& session = mSessions[sessionId];
	if ( session.waitingToAttach && mProtocol.runTarget && runTargetCb ) {
		runTargetCb();
		session.waitingToAttach = false;
	}
}

void DebuggerClientDap::processEventProcess( const nlohmann::json& body,
											 const SessionId& sessionId ) {
	ProcessInfo processInfo( body );
	for ( auto listener : mListeners )
		listener->debuggingProcess( processInfo, sessionId );
}

void DebuggerClientDap::processEventThread( const nlohmann::json& body,
											const SessionId& sessionId ) {
	ThreadEvent threadEvent( body );
	int threadId = threadEvent.threadId;
	if ( threadEvent.reason == "started" ) {
		mThreadToSession[threadId] = sessionId;
	} else if ( threadEvent.reason == "exited" ) {
		if ( mThreadToSession.count( threadId ) ) {
			mThreadToSession.erase( threadId );
		}
	}
	for ( auto listener : mListeners )
		listener->threadChanged( threadEvent, sessionId );
}

void DebuggerClientDap::processEventStopped( const nlohmann::json& body,
											 const SessionId& sessionId ) {
	StoppedEvent stoppedEvent( body );
	for ( auto listener : mListeners )
		listener->debuggeeStopped( stoppedEvent, sessionId );
}

void DebuggerClientDap::processEventModule( const nlohmann::json& body,
											const SessionId& sessionId ) {
	ModuleEvent moduleEvent( body );
	for ( auto listener : mListeners )
		listener->moduleChanged( moduleEvent, sessionId );
}

void DebuggerClientDap::processEventContinued( const nlohmann::json& body,
											   const SessionId& sessionId ) {
	ContinuedEvent continuedEvent( body );
	for ( auto listener : mListeners )
		listener->debuggeeContinued( continuedEvent, sessionId );
}

void DebuggerClientDap::processEventBreakpoint( const nlohmann::json& body,
												const SessionId& sessionId ) {
	BreakpointEvent breakpointEvent( body );
	for ( auto listener : mListeners )
		listener->breakpointChanged( breakpointEvent, sessionId );
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
			Log::error( "DebuggerClientDap::readHeader cannot parse header field: %s", header );
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
	std::string sessionId = findSessionByThread( threadId );
	if ( sessionId.empty() ) {
		Log::warning( "DebuggerClientDap::resume: no session for thread %d", threadId );
		return false;
	}
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest(
		"continue", arguments,
		[this, sessionId]( const Response& response, const nlohmann::json& request ) {
			if ( response.success ) {
				ContinuedEvent continuedEvent(
					request.value( DAP_THREAD_ID, 1 ),
					response.body.value( DAP_ALL_THREADS_CONTINUED, true ) );
				for ( auto listener : mListeners )
					listener->debuggeeContinued( continuedEvent, sessionId );
			}
		},
		sessionId );
	return true;
}

bool DebuggerClientDap::pause( int threadId ) {
	std::string sessionId = findSessionByThread( threadId );
	if ( sessionId.empty() )
		return false;
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	makeRequest( "pause", arguments, nullptr, sessionId );
	return true;
}

void DebuggerClientDap::processResponseNext( const Response& response,
											 const nlohmann::json& request,
											 const SessionId& sessionId ) {
	if ( response.success ) {
		bool all = false;
		if ( response.body.is_object() && response.body.contains( DAP_ALL_THREADS_CONTINUED ) )
			all = response.body.value( DAP_ALL_THREADS_CONTINUED, false );

		ContinuedEvent continuedEvent( request.value( DAP_THREAD_ID, 1 ), all );
		for ( auto listener : mListeners )
			listener->debuggeeContinued( continuedEvent, sessionId );
	}
}

bool DebuggerClientDap::stepOver( int threadId, bool singleThread ) {
	std::string sessionId = findSessionByThread( threadId );
	if ( sessionId.empty() )
		return false;
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest( "next", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this, sessionId ),
				 sessionId );
	return true;
}

bool DebuggerClientDap::goTo( int threadId, int targetId ) {
	std::string sessionId = findSessionByThread( threadId );
	if ( sessionId.empty() )
		return false;
	const nlohmann::json arguments{ { DAP_THREAD_ID, threadId }, { DAP_TARGET_ID, targetId } };
	makeRequest( "goto", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this, sessionId ),
				 sessionId );
	return true;
}

bool DebuggerClientDap::stepInto( int threadId, bool singleThread ) {
	std::string sessionId = findSessionByThread( threadId );
	if ( sessionId.empty() )
		return false;
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest( "stepIn", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this, sessionId ),
				 sessionId );
	return true;
}

bool DebuggerClientDap::stepOut( int threadId, bool singleThread ) {
	std::string sessionId = findSessionByThread( threadId );
	if ( sessionId.empty() )
		return false;
	nlohmann::json arguments{ { DAP_THREAD_ID, threadId } };
	if ( singleThread )
		arguments[DAP_SINGLE_THREAD] = true;
	makeRequest( "stepOut", arguments,
				 makeResponseHandler( &DebuggerClientDap::processResponseNext, this, sessionId ),
				 sessionId );
	return true;
}

bool DebuggerClientDap::terminate( bool restart ) {
	nlohmann::json arguments;
	if ( restart )
		arguments["restart"] = true;
	makeRequest( "terminate", arguments, nullptr, mCurrentSessionId );
	return true;
}

bool DebuggerClientDap::disconnect( bool terminateDebuggee, bool restart ) {
	nlohmann::json arguments;
	if ( mAdapterCapabilities.supportTerminateDebuggee && terminateDebuggee )
		arguments["terminateDebuggee"] = true;

	if ( restart )
		arguments["restart"] = true;

	makeRequest(
		"disconnect", arguments,
		[this]( const Response& response, const nlohmann::json& ) {
			if ( response.success ) {
				for ( auto listener : mListeners )
					listener->serverDisconnected( mCurrentSessionId );
			}
		},
		mCurrentSessionId );
	return true;
}

bool DebuggerClientDap::threads() {
	makeRequest(
		DAP_THREADS, {},
		[this]( const Response& response, const nlohmann::json& ) {
			if ( response.success && response.body.contains( DAP_THREADS ) ) {
				std::vector<DapThread> threads(
					DapThread::parseList( response.body[DAP_THREADS] ) );
				for ( auto listener : mListeners )
					listener->threads( std::move( threads ), mCurrentSessionId );
			} else {
				for ( auto listener : mListeners )
					listener->threads( {}, mCurrentSessionId );
			}
		},
		mCurrentSessionId );
	return true;
}

bool DebuggerClientDap::stackTrace( int threadId, int startFrame, int levels ) {
	const nlohmann::json arguments{
		{ DAP_THREAD_ID, threadId }, { "startFrame", startFrame }, { "levels", levels } };

	makeRequest(
		"stackTrace", arguments,
		[this]( const Response& response, const nlohmann::json& request ) {
			const int threadId = request.value( DAP_THREAD_ID, 1 );
			if ( response.success ) {
				StackTraceInfo stackTraceInfo( response.body );
				for ( auto listener : mListeners )
					listener->stackTrace( threadId, std::move( stackTraceInfo ),
										  mCurrentSessionId );
			} else {
				StackTraceInfo stackTraceInfo;
				for ( auto listener : mListeners )
					listener->stackTrace( threadId, std::move( stackTraceInfo ),
										  mCurrentSessionId );
			}
		},
		mCurrentSessionId );
	return true;
}

bool DebuggerClientDap::scopes( int frameId ) {
	const nlohmann::json arguments{ { DAP_FRAME_ID, frameId } };
	makeRequest(
		DAP_SCOPES, arguments,
		[this]( const Response& response, const nlohmann::json& request ) {
			const int frameId = request.value( DAP_FRAME_ID, 1 );
			if ( response.success && response.body.contains( DAP_SCOPES ) ) {
				auto scopes( Scope::parseList( response.body[DAP_SCOPES] ) );
				for ( auto listener : mListeners )
					listener->scopes( frameId, std::move( scopes ), mCurrentSessionId );
			} else {
				std::vector<Scope> scopes;
				for ( auto listener : mListeners )
					listener->scopes( frameId, std::move( scopes ), mCurrentSessionId );
			}
		},
		mCurrentSessionId );
	return true;
}

bool DebuggerClientDap::variables( int variablesReference, Variable::Type filter,
								   DebuggerClient::VariablesResponseCb responseCb, int start,
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

	makeRequest(
		DAP_VARIABLES, arguments,
		[this, responseCb = std::move( responseCb )]( const Response& response,
													  const nlohmann::json& request ) {
			const int variablesReference = request.value( DAP_VARIABLES_REFERENCE, 0 );
			if ( response.success && response.body.contains( DAP_VARIABLES ) ) {
				auto variableList = Variable::parseList( response.body[DAP_VARIABLES] );
				if ( responseCb ) {
					responseCb( variablesReference, std::move( variableList ) );
				} else if ( !mListeners.empty() ) {
					if ( mListeners.size() == 1 ) {
						// Move directly to single listener
						mListeners[0]->variables( variablesReference, std::move( variableList ),
												  mCurrentSessionId );
					} else {
						// Copy for multiple listeners
						for ( auto listener : mListeners ) {
							std::vector<Variable> variableListCopy = variableList;
							listener->variables( variablesReference, std::move( variableListCopy ),
												 mCurrentSessionId );
						}
					}
				}
			} else {
				std::vector<Variable> variableList; // Single empty vector
				if ( responseCb ) {
					responseCb( variablesReference, std::move( variableList ) );
				} else if ( !mListeners.empty() ) {
					// Share empty vector with all listeners
					for ( auto listener : mListeners ) {
						std::vector<Variable> variableListCopy = variableList;
						listener->variables( variablesReference, std::move( variableListCopy ),
											 mCurrentSessionId );
					}
				}
			}
		},
		mCurrentSessionId );

	return true;
}

bool DebuggerClientDap::modules( int start, int count ) {
	makeRequest(
		DAP_MODULES, { { DAP_START, start }, { DAP_COUNT, count } },
		[this]( const auto& response, const auto& ) {
			if ( response.success ) {
				ModulesInfo info( response.body );
				for ( auto listener : mListeners )
					listener->modules( std::move( info ), mCurrentSessionId );
			} else {
				ModulesInfo info;
				for ( auto listener : mListeners )
					listener->modules( std::move( info ), mCurrentSessionId );
			}
		},
		mCurrentSessionId );
	return true;
}

bool DebuggerClientDap::evaluate(
	const std::string& expression, const std::string& context, std::optional<int> frameId,
	std::function<void( const std::string&, const std::optional<EvaluateInfo>& )> cb ) {
	nlohmann::json arguments{ { DAP_EXPRESSION, expression } };
	if ( !context.empty() )
		arguments[DAP_CONTEXT] = context;
	if ( frameId )
		arguments[DAP_FRAME_ID] = *frameId;

	makeRequest(
		"evaluate", arguments,
		[this, cb = std::move( cb )]( const auto& response, const auto& request ) {
			auto expression = request.value( DAP_EXPRESSION, "" );
			if ( response.success ) {
				EvaluateInfo info( response.body );

				for ( auto listener : mListeners )
					listener->expressionEvaluated( expression, info, mCurrentSessionId );

				if ( cb )
					cb( expression, info );
			} else {
				for ( auto listener : mListeners )
					listener->expressionEvaluated( expression, std::nullopt, mCurrentSessionId );

				if ( cb )
					cb( expression, std::nullopt );
			}
		},
		mCurrentSessionId );

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

	makeRequest(
		"setBreakpoints", arguments,
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
													 breakpoints, mCurrentSessionId );
				} else {
					std::vector<Breakpoint> breakpoints;
					breakpoints.reserve( resp[DAP_LINES].size() );
					for ( const auto& item : resp[DAP_LINES] )
						breakpoints.emplace_back( item.get<int>() );

					for ( auto listener : mListeners )
						listener->sourceBreakpoints( source.path,
													 source.sourceReference.value_or( 0 ),
													 breakpoints, mCurrentSessionId );
				}
			} else {
				for ( auto listener : mListeners )
					listener->sourceBreakpoints( source.path, source.sourceReference.value_or( 0 ),
												 std::nullopt, mCurrentSessionId );
			}
		},
		mCurrentSessionId );

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

	makeRequest(
		"gotoTargets", arguments,
		[this]( const auto& response, const auto& req ) {
			const auto source = Source( req[DAP_SOURCE] );
			const int line = req.value( DAP_LINE, 1 );
			if ( response.success && response.body.contains( DAP_TARGETS ) ) {
				auto list = GotoTarget::parseList( response.body[DAP_TARGETS] );
				for ( auto listener : mListeners )
					listener->gotoTargets( source, line, list, mCurrentSessionId );
			} else {
				std::vector<GotoTarget> list;
				for ( auto listener : mListeners )
					listener->gotoTargets( source, line, list, mCurrentSessionId );
			}
		},
		mCurrentSessionId );

	return true;
}

bool DebuggerClientDap::watch( const std::string& expression, std::optional<int> frameId ) {
	return evaluate( expression, "watch", frameId );
}

std::string DebuggerClientDap::findSessionByThread( int threadId ) const {
	auto it = mThreadToSession.find( threadId );
	return it != mThreadToSession.end() ? it->second : mCurrentSessionId;
}

bool DebuggerClientDap::configurationDone( const SessionId& sessionId ) {
	Session& session = mSessions[sessionId];
	if ( session.state != State::Initialized ) {
		Log::warning( "DebuggerClientDap::configurationDone [Session %s]: unexpected state",
					  sessionId );
		return false;
	}

	if ( !mAdapterCapabilities.supportsConfigurationDoneRequest ) {
		session.configured = true;
		if ( session.launched && session.state == State::Initialized ) {
			setState( State::Running, sessionId );
			for ( auto listener : mListeners )
				listener->debuggeeRunning( sessionId );
		}
		for ( auto listener : mListeners )
			listener->configured( sessionId );
		return true;
	}

	makeRequest(
		"configurationDone", nlohmann::json{},
		[this, sessionId]( const auto& response, const auto& ) {
			Session& session = mSessions[sessionId];
			if ( response.success ) {
				session.configured = true;
				if ( session.launched && session.state == State::Initialized ) {
					setState( State::Running, sessionId );
					for ( auto listener : mListeners )
						listener->debuggeeRunning( sessionId );
				}
				for ( auto listener : mListeners )
					listener->configured( sessionId );
			}
		},
		sessionId );
	return true;
}

bool DebuggerClientDap::started() const {
	return mStarted;
}

void DebuggerClientDap::setState( const State& state, const SessionId& sessionId ) {
	if ( mSessions.count( sessionId ) == 0 ) {
		Log::warning(
			"DebuggerClientDap::setState [Session %s]: setting state of an invalid session",
			sessionId );
		return; // Invalid session
	}
	Session& session = mSessions[sessionId];
	if ( session.state != state ) {
		session.state = state;
		stateChanged( state, sessionId ); // Notify listeners
		switch ( state ) {
			case State::Initialized:
				initialized( sessionId );
				checkRunning( sessionId ); // Trigger running check
				break;
			case State::Running:
				// Additional logic for running state
				break;
			case State::Terminated:
				// Cleanup or notify termination
				break;
			case State::Failed:
				// Handle failure
				break;
			default:
				break;
		}
	}
}

void DebuggerClientDap::checkRunning( const SessionId& sessionId ) {
	if ( mSessions.count( sessionId ) == 0 ) {
		return; // Invalid session
	}
	Session& session = mSessions[sessionId];
	if ( session.launched && session.configured && session.state == State::Initialized ) {
		setState( State::Running, sessionId ); // Transition to Running state
	}
}

} // namespace ecode::dap
