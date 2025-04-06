#ifndef ECODE_DISCORDRPCPLUGIN_HPP
#define ECODE_DISCORDRPCPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "../git/gitplugin.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/threadpool.hpp>

#include "sdk/ipc.hpp"

#include <nlohmann/json.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Doc;

namespace ecode {

class DiscordRPCplugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "discrdrpc",
				 "Discord Rich Presence",
				 "Show your friends what you are up to through the discord Rich Presence system",
				 DiscordRPCplugin::New,
				 { 0, 0, 1 },
				 DiscordRPCplugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~DiscordRPCplugin() override;

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

  protected:
	DiscordIPC mIPC;
	Mutex mDataMutex;
	GitPlugin* mGitPlugin{ nullptr };

	std::string mLastFile;
	std::string mLastLang;
	std::string mLastLangName;
	std::string mProjectName;
	std::string mProjectPath;
	nlohmann::json mLangBindings;
	PluginManager* mPluginManager{ nullptr };
	bool mDoLangIcon;
	bool mDoGitIntegration;

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	virtual void onRegisterListeners( UICodeEditor* editor,
									  std::vector<Uint32>& listeners ) override;

	virtual void onRegisterEditor( UICodeEditor* editor ) override;

	virtual void onUnregisterEditor( UICodeEditor* editor ) override;

	DiscordRPCplugin( PluginManager* pluginManager, bool sync );

	void loadDiscordRPCConfig( const std::string& path, bool updateConfigFile );

	void initIPC();

	void updateActivity( DiscordIPCActivity& a );
};

} // namespace ecode

#endif // ECODE_DISCORDRPCPLUGIN_HPP
