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
		class DiscordRPCpluginClient : public TextDocument::Client {
			public:
				explicit DiscordRPCpluginClient( DiscordRPCplugin* parent, TextDocument* doc) :
					mDoc( doc ), mParent( parent ) {}
				
				// Obligatory definitions, empty behavior
				virtual void onDocumentTextChanged( const DocumentContentChange& ) override {};
				virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& ) override {};
				virtual void onDocumentCursorChange( const TextPosition& ) override;
				virtual void onDocumentInterestingCursorChange( const TextPosition& ) override {};
				virtual void onDocumentSelectionChange( const TextRange& ) override {};
				virtual void onDocumentLineCountChange( const size_t&, const size_t& ) override {};
				virtual void onDocumentLineChanged( const Int64& ) override {};
				virtual void onDocumentSaved( TextDocument* ) override {};
				virtual void onDocumentClosed( TextDocument* doc ) override {};
				virtual void onDocumentDirtyOnFileSystem( TextDocument* ) override {};
				virtual void onDocumentMoved( TextDocument* ) override {};
				virtual void onDocumentReset( TextDocument* ) override {};
				
					
			protected:
				TextDocument* mDoc{ nullptr };
				DiscordRPCplugin* mParent{ nullptr };
		};
		DiscordIPC mIPC;
		Mutex mIPCmutex;
		
		using ClientsMap = std::unordered_map<TextDocument*, std::unique_ptr<DiscordRPCpluginClient>>;
		ClientsMap mClients;
		Mutex mClientsMutex;
		std::string mLastFile;
		Mutex mLastFileMutex;
		
		
		void load ( PluginManager* pluginManager );
		
		virtual void onRegisterDocument( TextDocument* doc ) override;
		virtual void onUnregisterDocument( TextDocument* doc ) override;
		
		DiscordRPCplugin( PluginManager* pluginManager, bool sync );
};


} // namespace ecode

#endif // ECODE_DISCORDRPCPLUGIN_HPP