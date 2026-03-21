#include "acpclient.hpp"
#include <eepp/system/log.hpp>

namespace ecode { namespace acp {

ACPClient::ACPClient( std::shared_ptr<ThreadPool> threadPool, const Config& config ) :
	mThreadPool( threadPool ), mConfig( config ) {}

ACPClient::~ACPClient() {
	stop();
}

bool ACPClient::start() {
	auto flags = Process::getDefaultOptions() | Process::EnableAsync | Process::UseAbsolutePath;
	bool ret = mProcess.create( mConfig.command, mConfig.args, flags, mConfig.environment,
								mConfig.workingDirectory );
	if ( ret && mProcess.isAlive() ) {
		mProcess.startAsyncRead(
			[this]( const char* bytes, size_t n ) { readStdOut( bytes, n ); },
			[this]( const char* bytes, size_t n ) { readStdErr( bytes, n ); } );
		return true;
	}
	return false;
}

void ACPClient::stop() {
	mShuttingDown = true;
	if ( mProcess.isAlive() ) {
		mProcess.kill();
	}
}

bool ACPClient::isRunning() {
	return !mShuttingDown && mProcess.isAlive();
}

bool ACPClient::isReady() {
	return mReady;
}

void ACPClient::readStdOut( const char* bytes, size_t n ) {
	mReceiveBuffer.append( bytes, n );
	size_t pos;
	while ( ( pos = mReceiveBuffer.find( '\n' ) ) != std::string::npos ) {
		std::string line = mReceiveBuffer.substr( 0, pos );
		mReceiveBuffer.erase( 0, pos + 1 );
		if ( line.empty() || line == "\r" )
			continue;

		try {
			json msg = json::parse( line );
			Log::debug( "ACPClient RECV: %s", line.c_str() );
			processMessage( msg );
		} catch ( const json::exception& e ) {
			Log::error( "ACPClient JSON parse error: %s\nLine: %s", e.what(), line.c_str() );
		}
	}
}

void ACPClient::readStdErr( const char* bytes, size_t n ) {
	std::string err( bytes, n );
	Log::debug( "ACPClient stderr: %s", err.c_str() );
}

void ACPClient::processMessage( const json& msg ) {
	if ( msg.contains( "method" ) ) {
		if ( msg.contains( "id" ) ) {
			processRequest( msg );
		} else {
			processNotification( msg );
		}
	} else if ( msg.contains( "result" ) || msg.contains( "error" ) ) {
		processResponse( msg );
	}
}

void ACPClient::processRequest( const json& msg ) {
	std::string method = msg.value( "method", "" );
	json id = msg["id"];

	if ( method == "fs/read_text_file" && onReadTextFile ) {
		ReadTextFileRequest req( msg.value( "params", json::object() ) );
		onReadTextFile( req, [this, id]( const ReadTextFileResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else if ( method == "fs/write_text_file" && onWriteTextFile ) {
		WriteTextFileRequest req( msg.value( "params", json::object() ) );
		onWriteTextFile( req, [this, id]( const WriteTextFileResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else if ( method == "session/request_permission" && onRequestPermission ) {
		RequestPermissionRequest req( msg.value( "params", json::object() ) );
		onRequestPermission( req, [this, id]( const RequestPermissionResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else if ( method == "terminal/create" && onCreateTerminal ) {
		CreateTerminalRequest req( msg.value( "params", json::object() ) );
		onCreateTerminal( req, [this, id]( const CreateTerminalResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else if ( method == "terminal/output" && onTerminalOutput ) {
		TerminalOutputRequest req( msg.value( "params", json::object() ) );
		onTerminalOutput( req, [this, id]( const TerminalOutputResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else if ( method == "terminal/kill" && onKillTerminal ) {
		KillTerminalRequest req( msg.value( "params", json::object() ) );
		onKillTerminal( req, [this, id]( const KillTerminalResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else if ( method == "terminal/release" && onReleaseTerminal ) {
		ReleaseTerminalRequest req( msg.value( "params", json::object() ) );
		onReleaseTerminal( req, [this, id]( const ReleaseTerminalResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else if ( method == "terminal/wait_for_exit" && onWaitForTerminalExit ) {
		WaitForTerminalExitRequest req( msg.value( "params", json::object() ) );
		onWaitForTerminalExit( req, [this, id]( const WaitForTerminalExitResponse& res ) {
			sendResponse( id, res.toJson() );
		} );
	} else {
		sendError( id, -32601, "Method not found: " + method );
	}
}

void ACPClient::processNotification( const json& msg ) {
	std::string method = msg.value( "method", "" );
	if ( method == "session/update" && onSessionUpdate ) {
		auto params = msg.value( "params", json::object() );
		if ( params.contains( "update" ) ) {
			onSessionUpdate( params["update"] );
		} else {
			onSessionUpdate( params ); // Fallback if schema shifts or malformed
		}
	}
}

void ACPClient::processResponse( const json& msg ) {
	if ( !msg.contains( "id" ) || !msg["id"].is_number_integer() )
		return;
	IdType id = msg["id"].get<IdType>();

	JsonReplyHandler handler;
	{
		Lock l( mHandlersMutex );
		auto it = mHandlers.find( id );
		if ( it != mHandlers.end() ) {
			handler = it->second;
			mHandlers.erase( it );
		}
	}

	if ( handler ) {
		handler( id, msg );
	}
}

int ACPClient::write( json&& msg, const JsonReplyHandler& h ) {
	msg["jsonrpc"] = "2.0";
	int msgId = 0;

	if ( h ) {
		msgId = ++mLastMsgId;
		msg["id"] = msgId;
		Lock l( mHandlersMutex );
		mHandlers[msgId] = h;
	}

	std::string out = msg.dump() + "\n";
	if ( isRunning() ) {
		Log::debug( "ACPClient SEND: %s", out.c_str() );
		mProcess.write( out );
	}
	return msgId;
}

void ACPClient::sendResponse( const json& id, json&& result ) {
	json msg = { { "jsonrpc", "2.0" }, { "id", id }, { "result", result } };
	std::string out = msg.dump() + "\n";
	if ( isRunning() ) {
		Log::debug( "ACPClient SEND: %s", out.c_str() );
		mProcess.write( out );
	}
}

void ACPClient::sendError( const json& id, int code, const std::string& message ) {
	json msg = { { "jsonrpc", "2.0" },
				 { "id", id },
				 { "error", { { "code", code }, { "message", message } } } };
	std::string out = msg.dump() + "\n";
	if ( isRunning() ) {
		Log::debug( "ACPClient SEND: %s", out.c_str() );
		mProcess.write( out );
	}
}

void ACPClient::initialize( const InitializeRequest& req,
							const std::function<void( const InitializeResponse& )>& cb ) {
	write( { { "method", "initialize" }, { "params", req.toJson() } },
		   [this, cb]( const IdType&, const json& resp ) {
			   if ( resp.contains( "result" ) ) {
				   mReady = true;
				   if ( cb )
					   cb( InitializeResponse( resp["result"] ) );
			   }
		   } );
}

void ACPClient::newSession( const NewSessionRequest& req,
							const std::function<void( const NewSessionResponse& )>& cb ) {
	write( { { "method", "session/new" }, { "params", req.toJson() } },
		   [cb]( const IdType&, const json& resp ) {
			   if ( resp.contains( "result" ) && cb ) {
				   cb( NewSessionResponse( resp["result"] ) );
			   }
		   } );
}

void ACPClient::loadSession( const LoadSessionRequest& req,
							 const std::function<void( const LoadSessionResponse& )>& cb ) {
	write( { { "method", "session/load" }, { "params", req.toJson() } },
		   [cb]( const IdType&, const json& resp ) {
			   if ( resp.contains( "result" ) && cb ) {
				   cb( LoadSessionResponse( resp["result"] ) );
			   }
		   } );
}

void ACPClient::prompt( const PromptRequest& req,
						const std::function<void( const PromptResponse& )>& cb ) {
	write( { { "method", "session/prompt" }, { "params", req.toJson() } },
		   [cb]( const IdType&, const json& resp ) {
			   if ( resp.contains( "result" ) && cb ) {
				   cb( PromptResponse( resp["result"] ) );
			   }
		   } );
}

void ACPClient::cancel( const std::string& sessionId ) {
	write( { { "method", "session/cancel" }, { "params", { { "sessionId", sessionId } } } } );
}

}} // namespace ecode::acp
