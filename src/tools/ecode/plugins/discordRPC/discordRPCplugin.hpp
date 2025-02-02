#ifndef ECODE_DISCORDRPCPLUGIN_HPP
#define ECODE_DISCORDRPCPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/system/scopedop.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class DiscordRPCplugin : public Plugin {
	public:
		static PluginDefinition Definition() {
			return {
				"discrdrpc", // ID
				"Discord Rich Presence", // Title - Human name
				"Show your friends what you are up to through the discord Rich Presence system", // Description
				DiscordRPCplugin::New, // Creator function
				{ 0, 0, 0 }, // Version,
				DiscordRPCplugin::NewSync };
		}
		
		static Plugin* New( PluginManager* pluginManager );

		static Plugin* NewSync( PluginManager* pluginManager );
	
		virtual ~DiscordRPCplugin();
	
		std::string getId() { return Definition().id; }
	
		std::string getTitle() { return Definition().name; }
	
		std::string getDescription() { return Definition().description; }
		
		virtual String::HashType getConfigFileHash() { return mConfigHash; }

		void onRegister( UICodeEditor* );
	
		void onUnregister( UICodeEditor* );
		
		void update( UICodeEditor* );
	protected:
		std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
		std::unordered_set<TextDocument*> mDocs;
		std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
		std::unordered_map<TextDocument*, std::unique_ptr<Clock>> mDirtyDoc;
		std::map<std::string, std::string> mKeyBindings; /* cmd, shortcut */
		String::HashType mConfigHash{ 0 };
		Mutex mDocMutex;
		
		Clock mClock;
		
		DiscordRPCplugin( PluginManager* pluginManager, bool sync );
		
		void load( PluginManager* pluginManager );
		
		void loadConfig( const std::string& path, bool updateConfigFile );
		
		PluginRequestHandle processMessage( const PluginMessage& notification );
};

} // namespace ecode

#endif // ECODE_DISCORDRPCPLUGIN_HPP