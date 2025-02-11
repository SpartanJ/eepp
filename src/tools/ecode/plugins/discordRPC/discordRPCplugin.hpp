#ifndef ECODE_DISCORDRPCPLUGIN_HPP
#define ECODE_DISCORDRPCPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"

#include <eepp/system/threadpool.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>

#include "sdk/ipc.hpp"

#include <nlohmann/json.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Doc;

#define DISCORDRPC_DEFAULT_ICON "https://github.com/SpartanJ/eepp/blob/develop/bin/assets/icon/ecode.png?raw=true"

namespace ecode {

class DiscordRPCplugin : public PluginBase {
	public:
		static PluginDefinition Definition() {
			return {
				"discrdrpc", 
				"Discord Rich Presence", 
				"Show your friends what you are up to through the discord Rich Presence system", 
				DiscordRPCplugin::New,
				{ 0, 0, 0 },
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
		std::string mLastFile;
		std::string mProjectName;
		nlohmann::json mcLangBindings;
		
		void load ( PluginManager* pluginManager );
		
		PluginRequestHandle processMessage( const PluginMessage& msg );
		
		virtual void onRegisterListeners(UICodeEditor* editor, std::vector<Uint32>& listeners ) override;
		virtual void onRegisterDocument( TextDocument* doc ) override;
		
		
		DiscordRPCplugin( PluginManager* pluginManager, bool sync );
};


} // namespace ecode

#endif // ECODE_DISCORDRPCPLUGIN_HPP