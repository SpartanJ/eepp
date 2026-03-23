#pragma once

#include "acpclient.hpp"
#include <eepp/system/threadpool.hpp>
#include <eterm/terminal/terminalemulator.hpp>
#include <eterm/ui/uiterminal.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

using namespace EE::System;
using namespace eterm::Terminal;
using namespace eterm::UI;

namespace ecode { namespace acp {

class AgentSession {
  public:
	AgentSession( std::shared_ptr<ThreadPool> threadPool, const ACPClient::Config& config );
	~AgentSession();

	bool start( const std::function<void( bool )>& onReady );
	bool startLoaded( const std::string& sessionId, const std::function<void( bool )>& onReady );
	void listSessions( const std::function<void( const std::vector<SessionInfo>&,
												 const std::optional<ResponseError>& )>& cb );
	void stop();

	void prompt( const PromptRequest& req,
				 const std::function<void( const PromptResponse&,
										   const std::optional<ResponseError>& )>& cb );
	void cancel();

	bool isPrompting() const { return mIsPrompting; }

	std::function<void( const ResponseError& )> onError;
	std::function<void( const json& )> onSessionUpdate;
	std::function<void( const RequestPermissionRequest&,
						std::function<void( const RequestPermissionResponse& )> )>
		onRequestPermission;
	std::function<void( const CreateTerminalRequest&, const std::string& terminalId )>
		onTerminalCreated;

	std::string getSessionId() const { return mSessionId; }
	ACPClient* getClient() const { return mClient.get(); }
	json getConfigOptions() const { return mConfigOptions; }
	void setConfigOptions( const json& opts ) { mConfigOptions = opts; }

	void setTerminalData( const std::string& terminalId, UITerminal* uiTerm );

  protected:
	std::shared_ptr<ThreadPool> mThreadPool;
	std::unique_ptr<ACPClient> mClient;
	std::string mSessionId;
	json mConfigOptions;
	bool mIsPrompting{ false };

	struct TermData {
		std::shared_ptr<TerminalDisplay> display;
		std::shared_ptr<TerminalEmulator> emulator;
		UITerminal* uiTerm{ nullptr };
	};
	std::unordered_map<std::string, TermData> mTerminals;

	void setupClient();
};

}} // namespace ecode::acp
