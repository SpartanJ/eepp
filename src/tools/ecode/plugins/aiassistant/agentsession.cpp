#include "agentsession.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>

namespace ecode { namespace acp {

AgentSession::AgentSession( std::shared_ptr<ThreadPool> threadPool,
							const ACPClient::Config& config ) :
	mThreadPool( threadPool ), mClient( std::make_unique<ACPClient>( threadPool, config ) ) {
	setupClient();
}

AgentSession::~AgentSession() {
	stop();
}

bool AgentSession::start( const std::function<void( bool )>& onReady ) {
	if ( mClient->start() ) {
		InitializeRequest req;
		req.clientCapabilities.terminal = true;
		req.clientCapabilities.fsReadTextFile = true;
		req.clientCapabilities.fsWriteTextFile = true;

		mClient->initialize( req, [this, onReady]( const InitializeResponse& ) {
			NewSessionRequest nreq;
			nreq.cwd = mClient->isReady() ? mClient->getConfig().workingDirectory : "";
			mClient->newSession( nreq, [this, onReady]( const NewSessionResponse& nres ) {
				mSessionId = nres.sessionId;
				if ( onReady )
					onReady( true );
			} );
		} );
		return true;
	}
	if ( onReady )
		onReady( false );
	return false;
}

void AgentSession::stop() {
	if ( mClient )
		mClient->stop();
}

void AgentSession::prompt( const PromptRequest& req,
						   const std::function<void( const PromptResponse& )>& cb ) {
	mIsPrompting = true;
	mClient->prompt( req, [this, cb](const PromptResponse& res) {
		mIsPrompting = false;
		if ( cb )
			cb(res);
	} );
}

void AgentSession::cancel() {
	if ( mClient && !mSessionId.empty() ) {
		mClient->cancel( mSessionId );
	}
}

void AgentSession::setupClient() {
	mClient->onSessionUpdate = [this]( const json& msg ) {
		if ( onSessionUpdate )
			onSessionUpdate( msg );
	};

	mClient->onReadTextFile = []( const ReadTextFileRequest& req, auto cb ) {
		ReadTextFileResponse res;
		std::string content;
		if ( FileSystem::fileGet( req.path, content ) ) {
			res.content = content;
		}
		cb( res );
	};

	mClient->onWriteTextFile = []( const WriteTextFileRequest& req, auto cb ) {
		FileSystem::fileWrite( req.path, req.content );
		cb( WriteTextFileResponse() );
	};

	mClient->onRequestPermission = [this]( const RequestPermissionRequest& req, auto cb ) {
		if ( onRequestPermission ) {
			onRequestPermission( req, cb );
		} else {
			RequestPermissionResponse res;
			res.outcome = "rejected";
			cb( res );
		}
	};

	mClient->onCreateTerminal = [this]( const CreateTerminalRequest& req, auto cb ) {
		CreateTerminalResponse res;
		std::string termId = String::format( "term-%u", String::hash( req.command ) );
		res.terminalId = termId;
		// Wait for UI? No, ACPClient is running in threads. We just trigger the event
		// and return the ID.
		if ( onTerminalCreated ) {
			onTerminalCreated( nullptr, termId );
		}
		cb( res );
	};

	mClient->onTerminalOutput = [this]( const TerminalOutputRequest& req, auto cb ) {
		TerminalOutputResponse res;
		res.output = "";
		res.truncated = false;
		auto it = mTerminals.find( req.terminalId );
		if ( it != mTerminals.end() && it->second.emulator ) {
			if ( it->second.emulator->hasExited() ) {
				TerminalExitStatus status;
				status.exitCode = it->second.emulator->getExitCode();
				res.exitStatus = status;
			}
		}
		cb( res );
	};

	mClient->onKillTerminal = [this]( const KillTerminalRequest& req, auto cb ) {
		auto it = mTerminals.find( req.terminalId );
		if ( it != mTerminals.end() && it->second.emulator ) {
			it->second.emulator->terminate();
		}
		cb( KillTerminalResponse() );
	};

	mClient->onReleaseTerminal = [this]( const ReleaseTerminalRequest& req, auto cb ) {
		auto it = mTerminals.find( req.terminalId );
		if ( it != mTerminals.end() ) {
			if ( it->second.emulator )
				it->second.emulator->terminate();
			if ( it->second.uiTerm )
				it->second.uiTerm->close();
			mTerminals.erase( it );
		}
		cb( ReleaseTerminalResponse() );
	};

	mClient->onWaitForTerminalExit = [this]( const WaitForTerminalExitRequest& req, auto cb ) {
		WaitForTerminalExitResponse res;
		auto it = mTerminals.find( req.terminalId );
		if ( it != mTerminals.end() && it->second.emulator ) {
			if ( it->second.emulator->hasExited() ) {
				res.exitCode = it->second.emulator->getExitCode();
			}
		}
		cb( res );
	};
}

}} // namespace ecode::acp
