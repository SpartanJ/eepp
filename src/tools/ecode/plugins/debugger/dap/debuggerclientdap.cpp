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

DebuggerClientDap::DebuggerClientDap( std::unique_ptr<Bus>&& bus ) : mBus( std::move( bus ) ) {}

void DebuggerClientDap::makeRequest( const std::string_view& command,
									 const nlohmann::json& arguments, ResponseHandler onFinish ) {
	nlohmann::json jsonCmd = {
		{ "seq", mIdx.load() },
		{ "type", "request" },
		{ "command", command },
		{ "arguments", arguments.empty() ? nlohmann::json::object() : arguments } };

	std::string cmd = jsonCmd.dump();

	Log::instance()->writel( mDebug ? LogLevel::Info : LogLevel::Debug, cmd );

	std::string msg( String::format( "Content-Length: %zu\r\n\r\n%s", cmd.size(), cmd ) );
	mBus->write( msg.data(), msg.size() );

	mRequests[mIdx] = { std::string{ command }, arguments, onFinish };

	mIdx.fetch_add( 1, std::memory_order_relaxed );
}

bool DebuggerClientDap::hasBreakpoint( const std::string& path, size_t line ) {
	return false;
}

bool DebuggerClientDap::addBreakpoint( const std::string& path, size_t line ) {
	return false;
}

bool DebuggerClientDap::removeBreakpoint( const std::string& path, size_t line ) {
	return false;
}

bool DebuggerClientDap::start() {
	bool started = mBus->start();
	if ( started )
		mBus->startAsyncRead( [this]( const char* bytes, size_t n ) { asyncRead( bytes, n ); } );
	mStarted = started;

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
	if ( mObserver )
		mObserver->capabilitiesReceived( mAdapterCapabilities );

	requestLaunchCommand();
}

void DebuggerClientDap::processResponseLaunch( const Response& response, const nlohmann::json& ) {
	if ( response.success ) {
		mLaunched = true;
		if ( mObserver )
			mObserver->launched();
		checkRunning();
	} else {
		setState( State::Failed );
	}
}

void DebuggerClientDap::requestLaunchCommand() {

	if ( mState != State::Initializing ) {
		Log::warning(
			"DebuggerClientDap::requestLaunchCommand: trying to launch in an unexpected state" );
		return;
	}

	if ( mLaunchCommand.empty() )
		return;

	makeRequest( mLaunchCommand, mProtocol.launchRequest,
				 makeResponseHandler( &DebuggerClientDap::processResponseLaunch, this ) );
}

void DebuggerClientDap::requestInitialize() {
	const nlohmann::json capabilities{
		{ DAP_CLIENT_ID, "ecode-dap" },
		{ DAP_CLIENT_NAME, "ecode dap" },
		{ "locale", mProtocol.locale },
		{ DAP_ADAPTER_ID, "qdap" },
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

			if ( mDebug )
				Log::debug( message.dump() );

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
	if ( mObserver )
		mObserver->errorResponse( summary, message );
}

void DebuggerClientDap::processEvent( const nlohmann::json& msg ) {
	const std::string event = msg.value( DAP_EVENT, "" );
	const auto body = msg[DAP_BODY];

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
}

void DebuggerClientDap::processEventTerminated() {
	setState( State::Terminated );
}

void DebuggerClientDap::processEventExited( const nlohmann::json& body ) {
	const int exitCode = body.value( "exitCode", -1 );
	if ( mObserver )
		mObserver->debuggeeExited( exitCode );
}

void DebuggerClientDap::processEventOutput( const nlohmann::json& body ) {
	if ( mObserver )
		mObserver->outputProduced( Output( body ) );
}

void DebuggerClientDap::processEventProcess( const nlohmann::json& body ) {
	if ( mObserver )
		mObserver->debuggingProcess( ProcessInfo( body ) );
}

void DebuggerClientDap::processEventThread( const nlohmann::json& body ) {
	if ( mObserver )
		mObserver->threadChanged( ThreadEvent( body ) );
}

void DebuggerClientDap::processEventStopped( const nlohmann::json& body ) {
	if ( mObserver )
		mObserver->debuggeeStopped( StoppedEvent( body ) );
}

void DebuggerClientDap::processEventModule( const nlohmann::json& body ) {
	if ( mObserver )
		mObserver->moduleChanged( ModuleEvent( body ) );
}

void DebuggerClientDap::processEventContinued( const nlohmann::json& body ) {
	if ( mObserver )
		mObserver->debuggeeContinued( ContinuedEvent( body ) );
}

void DebuggerClientDap::processEventBreakpoint( const nlohmann::json& body ) {
	if ( mObserver )
		mObserver->breakpointChanged( BreakpointEvent( body ) );
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
			Uint64 length;
			if ( !String::fromString( length, lengthStr ) ) {
				Log::error( "DebuggerClientDap::readHeader invalid value: ", header );
				discardExploredBuffer();
				continue; // CONTINUE HEADER
			}
		}

		start = end;
	}

	if ( length < 0 )
		return std::nullopt;

	return HeaderInfo{ end, length };
}

bool DebuggerClientDap::attach() {
	return false;
}

bool DebuggerClientDap::started() const {
	return mStarted;
}

bool DebuggerClientDap::cont( int threadId ) {
	return false;
}

bool DebuggerClientDap::pause( int threadId ) {
	return false;
}

bool DebuggerClientDap::next( int threadId ) {
	return false;
}

bool DebuggerClientDap::goTo( int threadId, int targetId ) {
	return false;
}

bool DebuggerClientDap::stepInto( int threadId ) {
	return false;
}

bool DebuggerClientDap::stepOver( int threadId ) {
	return false;
}

bool DebuggerClientDap::stepOut( int threadId ) {
	return false;
}

bool DebuggerClientDap::halt() {
	return false;
}

bool DebuggerClientDap::terminate() {
	return false;
}

bool DebuggerClientDap::stopped() {
	return false;
}

bool DebuggerClientDap::completed() {
	return false;
}

} // namespace ecode::dap
