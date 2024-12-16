#include "debuggerclientdap.hpp"
#include "messages.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/log.hpp>

using namespace EE::System;

namespace ecode::dap {

constexpr int MAX_HEADER_SIZE = 1 << 16;

DebuggerClientDap::DebuggerClientDap( std::unique_ptr<Bus>&& bus ) : mBus( std::move( bus ) ) {}

void DebuggerClientDap::makeRequest( const std::string& command, const nlohmann::json& arguments,
									 std::function<void()> onFinish ) {
	nlohmann::json json_cmd = {
		{ "seq", mIdx.load() },
		{ "type", "request" },
		{ "command", command },
		{ "arguments", arguments.empty() ? nlohmann::json::object() : arguments } };

	std::string cmd = json_cmd.dump();

	Log::instance()->writel( mDebug ? LogLevel::Info : LogLevel::Debug, cmd );

	std::string msg( String::format( "Content-Length: %zu\r\n\r\n%s", cmd.size(), cmd ) );
	mBus->write( msg.data(), msg.size() );

	if ( onFinish )
		mCommandQueue[mIdx] = onFinish;

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

void DebuggerClientDap::processResponse( const nlohmann::json& msg ) {}

void DebuggerClientDap::processEvent( const nlohmann::json& msg ) {}

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

	return HeaderInfo{ .payloadStart = end, .payloadLength = length };
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
