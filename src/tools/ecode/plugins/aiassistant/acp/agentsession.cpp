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

		mClient->initialize( req, [this, onReady]( const InitializeResponse&,
												   const std::optional<ResponseError>& err ) {
			if ( err ) {
				if ( onError )
					onError( *err );
				if ( onReady )
					onReady( false );
				return;
			}
			NewSessionRequest nreq;
			nreq.cwd = mClient->isReady() ? mClient->getConfig().workingDirectory : "";
			mClient->newSession( nreq, [this, onReady]( const NewSessionResponse& nres,
														const std::optional<ResponseError>& err ) {
				if ( err ) {
					if ( onError )
						onError( *err );
					if ( onReady )
						onReady( false );
					return;
				}
				mSessionId = nres.sessionId;
				mConfigOptions = nres.configOptions;
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

bool AgentSession::startLoaded( const std::string& sessionId,
								const std::function<void( bool )>& onReady ) {
	if ( mClient->start() ) {
		InitializeRequest req;
		req.clientCapabilities.terminal = true;
		req.clientCapabilities.fsReadTextFile = true;
		req.clientCapabilities.fsWriteTextFile = true;

		mClient->initialize( req, [this, sessionId,
								   onReady]( const InitializeResponse& ires,
											 const std::optional<ResponseError>& err ) {
			if ( err ) {
				if ( onError )
					onError( *err );
				if ( onReady )
					onReady( false );
				return;
			}
			if ( ires.agentCapabilities.loadSession ) {
				LoadSessionRequest lreq;
				lreq.sessionId = sessionId;
				lreq.cwd = mClient->isReady() ? mClient->getConfig().workingDirectory : "";
				mClient->loadSession(
					lreq, [this, sessionId, onReady]( const LoadSessionResponse& lres,
													  const std::optional<ResponseError>& err ) {
						if ( err ) {
							if ( onError )
								onError( *err );
							if ( onReady )
								onReady( false );
							return;
						}
						mSessionId = sessionId;
						mConfigOptions = lres.configOptions;
						if ( onReady )
							onReady( true );
					} );
			} else {
				if ( onError )
					onError( { -1, "Agent does not support loading sessions" } );
				if ( onReady )
					onReady( false );
			}
		} );
		return true;
	}
	if ( onReady )
		onReady( false );
	return false;
}

void AgentSession::listSessions(
	const std::function<void( const std::vector<SessionInfo>&,
							  const std::optional<ResponseError>& )>& cb ) {
	if ( !mClient->isReady() ) {
		if ( cb )
			cb( {}, std::nullopt );
		return;
	}
	ListSessionsRequest req;
	req.cwd = mClient->getConfig().workingDirectory;
	mClient->listSessions( req, [this, cb]( const ListSessionsResponse& res,
											const std::optional<ResponseError>& err ) {
		if ( err && onError )
			onError( *err );
		if ( cb )
			cb( res.sessions, err );
	} );
}

void AgentSession::stop() {
	for ( auto& term : mTerminals ) {
		if ( term.second.display && term.second.eventCbId )
			term.second.display->popEventCallback( term.second.eventCbId );
	}
	mTerminals.clear();

	if ( mClient )
		mClient->stop();
}

void AgentSession::prompt(
	const PromptRequest& req,
	const std::function<void( const PromptResponse&, const std::optional<ResponseError>& )>& cb ) {
	mIsPrompting = true;
	mClient->prompt(
		req, [this, cb]( const PromptResponse& res, const std::optional<ResponseError>& err ) {
			mIsPrompting = false;
			if ( err && onError )
				onError( *err );
			if ( cb )
				cb( res, err );
		} );
}

void AgentSession::setConfigOption(
	const SetConfigOptionRequest& req,
	const std::function<void( const SetConfigOptionResponse&,
							  const std::optional<ResponseError>& )>& cb ) {
	mClient->setConfigOption( req, [this, cb]( const SetConfigOptionResponse& res,
											   const std::optional<ResponseError>& err ) {
		if ( err && onError )
			onError( *err );
		if ( cb )
			cb( res, err );
	} );
}

void AgentSession::cancel() {
	if ( mClient && !mSessionId.empty() ) {
		mClient->cancel( mSessionId );
	}
}

void AgentSession::setTerminalData( const std::string& terminalId, UITerminal* uiTerm ) {
	auto& termData = mTerminals[terminalId];
	termData.display = uiTerm->getTerm();
	termData.emulator = uiTerm->getTerm()->getTerminal();
	termData.uiTerm = uiTerm;
	if ( termData.emulator ) {
		termData.emulator->setDataCb( [this, terminalId]( const char* data, size_t size ) {
			auto it = mTerminals.find( terminalId );
			if ( it != mTerminals.end() ) {
				it->second.outputBuffer.append( data, size );
			}
		} );
	}
	if ( termData.display ) {
		termData.eventCbId =
			termData.display->pushEventCallback( [this, terminalId]( const auto& event ) {
				if ( event.type == TerminalDisplay::EventType::PROCESS_EXIT ) {
					auto it = mTerminals.find( terminalId );
					if ( it != mTerminals.end() ) {
						WaitForTerminalExitResponse res;
						res.exitCode = it->second.emulator ? it->second.emulator->getExitCode() : 0;
						auto callbacks = std::move( it->second.exitCallbacks );
						it->second.exitCallbacks.clear();
						for ( const auto& cb : callbacks ) {
							cb( res );
						}
					}
				}
			} );
	}
}

void AgentSession::setupClient() {
	mClient->onError = [this]( const ResponseError& err ) {
		if ( onError )
			onError( err );
	};

	mClient->onSessionUpdate = [this]( const json& msg ) {
		auto sessionUpdate = msg.value( "sessionUpdate", "" );
		if ( sessionUpdate == "config_options_update" && msg.contains( "configOptions" ) ) {
			mConfigOptions = msg["configOptions"];
		} else if ( msg.contains( "models" ) || msg.contains( "modes" ) ) {
			mConfigOptions = parseLegacyConfigOptions( msg, mConfigOptions );
		}
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
		if ( onTerminalCreated ) {
			onTerminalCreated( req, termId );
		}
		cb( res );
	};

	mClient->onTerminalOutput = [this]( const TerminalOutputRequest& req, auto cb ) {
		TerminalOutputResponse res;
		res.output = "";
		res.truncated = false;
		auto it = mTerminals.find( req.terminalId );
		if ( it != mTerminals.end() ) {
			size_t limit = req.outputByteLimit.value_or( 0 );
			if ( limit > 0 && it->second.outputBuffer.size() > limit ) {
				res.output = it->second.outputBuffer.substr( 0, limit );
				it->second.outputBuffer.erase( 0, limit );
				res.truncated = true;
			} else {
				res.output = std::move( it->second.outputBuffer );
				it->second.outputBuffer.clear();
				res.truncated = false;
			}

			if ( it->second.emulator && it->second.emulator->hasExited() ) {
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
			if ( it->second.display && it->second.eventCbId )
				it->second.display->popEventCallback( it->second.eventCbId );
			if ( it->second.emulator )
				it->second.emulator->terminate();
			if ( it->second.uiTerm )
				it->second.uiTerm->close();
			mTerminals.erase( it );
		}
		cb( ReleaseTerminalResponse() );
	};

	mClient->onWaitForTerminalExit = [this]( const WaitForTerminalExitRequest& req, auto cb ) {
		auto it = mTerminals.find( req.terminalId );
		if ( it != mTerminals.end() && it->second.emulator ) {
			if ( it->second.emulator->hasExited() ) {
				WaitForTerminalExitResponse res;
				res.exitCode = it->second.emulator->getExitCode();
				cb( res );
			} else {
				it->second.exitCallbacks.push_back( cb );
			}
		} else {
			cb( WaitForTerminalExitResponse() );
		}
	};
}

}} // namespace ecode::acp
