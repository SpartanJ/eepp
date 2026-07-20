#include "autocompleteplugin.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiplacementutils.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>

#include <algorithm>
#include <nlohmann/json.hpp>
using namespace EE::Graphics;
using namespace EE::System;
using json = nlohmann::json;
using namespace std::literals;

namespace ecode {

class AutoCompletePlugin::SnippetDocumentClient : public TextDocument::Client {
  public:
	SnippetDocumentClient( AutoCompletePlugin* plugin, TextDocument* doc ) :
		mPlugin( plugin ), mDoc( doc ) {}

	void detach() {
		if ( mDoc ) {
			mDoc->unregisterClient( this );
			mDoc = nullptr;
		}
	}

	bool isAttached() const { return mDoc != nullptr; }

	void onDocumentTextChanged( const DocumentContentChange& change ) override {
		if ( mDoc )
			mPlugin->onSnippetTextChanged( mDoc, change );
	}

	void onDocumentUndoRedo( const TextDocument::UndoRedo& ) override {
		if ( mDoc )
			mPlugin->cancelSnippetSession( mDoc );
	}

	void onDocumentCursorChange( const TextPosition& ) override {}

	void onDocumentSelectionChange( const TextRange& ) override {
		if ( mDoc )
			mPlugin->onSnippetSelectionChanged( mDoc );
	}

	void onDocumentLineCountChange( const size_t&, const size_t& ) override {}
	void onDocumentLineChanged( const Int64& ) override {}
	void onDocumentSaved( TextDocument* ) override {}

	void onDocumentClosed( TextDocument* doc ) override {
		mPlugin->onSnippetDocumentClosed( doc );
		mDoc = nullptr;
	}

	void onDocumentDirtyOnFileSystem( TextDocument* ) override {}
	void onDocumentMoved( TextDocument* ) override {}

	void onDocumentReset( TextDocument* doc ) override { mPlugin->cancelSnippetSession( doc ); }

	Client::Type getTextDocumentClientType() override { return Client::Auxiliary; }

  private:
	AutoCompletePlugin* mPlugin{ nullptr };
	TextDocument* mDoc{ nullptr };
};

static json getURIJSON( TextDocument* doc, const PluginIDType& id ) {
	json data;
	data["uri"] = doc->getURI().toString();
	if ( id.isInteger() )
		data["id"] = id.asInt();
	else
		data["id"] = id.asString();
	return data;
}

static json getURIAndPositionJSON( UICodeEditor* editor ) {
	json data;
	auto doc = editor->getDocumentRef();
	auto sel = doc->getSelection();
	data["uri"] = doc->getURI().toString();
	data["position"] = { { "line", sel.start().line() }, { "character", sel.start().column() } };
	return data;
}

static AutoCompletePlugin::SymbolsList
fuzzyMatchSymbols( const std::vector<const AutoCompletePlugin::SymbolsList*>& symbolsVec,
				   const std::string& pattern, const size_t& max ) {
	AutoCompletePlugin::SymbolsList matches;
	matches.reserve( max );
	int score = 0;
	for ( const auto& symbols : symbolsVec ) {
		for ( const auto& symbol : *symbols ) {
			if ( symbol.kind == LSPCompletionItemKind::Snippet ||
				 ( score = String::fuzzyMatchSimple(
					   pattern, symbol.text, false, symbol.kind != LSPCompletionItemKind::Text ) ) >
					 0 ) {
				if ( std::find( matches.begin(), matches.end(), symbol ) == matches.end() ) {
					symbol.setScore( score +
									 ( symbol.kind != LSPCompletionItemKind::Text ? score : 0 ) );
					matches.push_back( symbol );

					if ( matches.size() >= max )
						break;
				}
			}
		}

		if ( matches.size() >= max )
			break;
	}

	std::sort(
		matches.begin(), matches.end(),
		[]( const AutoCompletePlugin::Suggestion& left,
			const AutoCompletePlugin::Suggestion& right ) { return left.score > right.score; } );

	return matches;
}

Plugin* AutoCompletePlugin::New( PluginManager* pluginManager ) {
	return eeNew( AutoCompletePlugin, ( pluginManager, false ) );
}

Plugin* AutoCompletePlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( AutoCompletePlugin, ( pluginManager, true ) );
}

AutoCompletePlugin::AutoCompletePlugin( PluginManager* pluginManager, bool sync ) :
	Plugin( pluginManager ),
	mSymbolPattern( "[%a_ñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ][%w_"
					"ñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ]*" ),
	mBoxPadding( PixelDensity::dpToPx( Rectf( 4, 4, 12, 4 ) ) ) {
	mManager->subscribeMessages( this, [this]( const PluginMessage& msg ) -> PluginRequestHandle {
		return processResponse( msg );
	} );
	if ( sync ) {
		load( pluginManager );
	} else {
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
	}
}

AutoCompletePlugin::~AutoCompletePlugin() {
	waitUntilLoaded();
	mShuttingDown = true;
	mManager->unsubscribeMessages( this );
	unsubscribeFileSystemListener();
	for ( auto& client : mSnippetClients )
		client.second->detach();
	mSnippetClients.clear();
	mSnippetSessions.clear();

	{
		Lock l( mDocMutex );
		Lock l2( mLangSymbolsMutex );
		Lock l3( mSuggestionsMutex );
		for ( const auto& editor : mEditors ) {
			for ( auto listener : editor.second )
				editor.first->removeEventListener( listener );
			editor.first->unregisterPlugin( this );
		}
	}

	bool isUpdating = false;
	do {
		{
			Lock lu( mDocsUpdatingMutex );
			isUpdating = std::any_of( mDocsUpdating.begin(), mDocsUpdating.end(),
									  []( const auto& du ) { return du.second == true; } );
		}
		if ( isUpdating )
			Sys::sleep( Milliseconds( 1 ) );
	} while ( isUpdating );
}

void AutoCompletePlugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
	std::string path = pluginManager->getPluginsPath() + "autocomplete.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n  \"config\":{},\n  \"keybindings\":{}\n}\n" ) ) {
		mConfigPath = path;
	}
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	mConfigHash = String::hash( data );

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error(
			"AutoCompletePlugin::load - Error parsing config from path %s, error: %s, config "
			"file content:\n%s",
			path, e.what(), data );
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n}\n", nullptr, true, true );
	}

	bool updateConfigFile = false;

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "suggestions_syntax_highlight" ) )
			mHighlightSuggestions = config.value( "suggestions_syntax_highlight", true );
		else {
			config["suggestions_syntax_highlight"] = mHighlightSuggestions;
			updateConfigFile = true;
		}

		if ( config.contains( "max_label_characters" ) )
			mMaxLabelCharacters = config.value( "max_label_characters", 100 );
		else {
			config["max_label_characters"] = mMaxLabelCharacters;
			updateConfigFile = true;
		}

		if ( config.contains( "max_suggestion_documentation_width" ) )
			mMaxSuggestionDocumentationWidth =
				config.value( "max_suggestion_documentation_width", "100%" );
		else {
			config["max_suggestion_documentation_width"] = "100%";
			updateConfigFile = true;
		}

		if ( mMaxSuggestionDocumentationWidth.empty() )
			mMaxSuggestionDocumentationWidth = "100%";

		if ( config.contains( "max_signature_helper_width" ) )
			mMaxSignatureHelperWidth = config.value( "max_signature_helper_width", "90%" );
		else {
			config["max_signature_helper_width"] = "90%";
			updateConfigFile = true;
		}

		if ( mMaxSignatureHelperWidth.empty() )
			mMaxSignatureHelperWidth = "90%";

		if ( config.contains( "signature_help_multi_line" ) )
			mSignatureHelpMultiLine = config.value( "signature_help_multi_line", true );
		else {
			config["signature_help_multi_line"] = mSignatureHelpMultiLine;
			updateConfigFile = true;
		}

		if ( config.contains( "suggestion_documentation" ) )
			mSuggestionDocumentation = config.value( "suggestion_documentation", true );
		else {
			config["suggestion_documentation"] = mSuggestionDocumentation;
			updateConfigFile = true;
		}

		if ( config.contains( "signature_help_documentation" ) )
			mSignatureHelpDocumentation = config.value( "signature_help_documentation", true );
		else {
			config["suggestion_documentation"] = mSignatureHelpDocumentation;
			updateConfigFile = true;
		}
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["autocomplete-close-suggestion"] = "escape";
		mKeyBindings["autocomplete-prev-suggestion"] = "up";
		mKeyBindings["autocomplete-next-suggestion"] = "down";
		mKeyBindings["autocomplete-first-suggestion"] = "home";
		mKeyBindings["autocomplete-last-suggestion"] = "end";
		mKeyBindings["autocomplete-prev-suggestion-page"] = "pageup";
		mKeyBindings["autocomplete-next-suggestion-page"] = "pagedown";
		mKeyBindings["autocomplete-pick-suggestion"] = "tab";
		mKeyBindings["autocomplete-pick-suggestion-alt"] = "enter";
		mKeyBindings["autocomplete-pick-suggestion-alt-2"] = "return";
		mKeyBindings["autocomplete-update-suggestions"] = "mod+space";
		mKeyBindings["autocomplete-close-signature-help"] = "escape";
		mKeyBindings["autocomplete-prev-signature-help"] = "up";
		mKeyBindings["autocomplete-next-signature-help"] = "down";
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		// clang-format off
		auto list = {
			"autocomplete-close-suggestion",
			"autocomplete-prev-suggestion",
			"autocomplete-next-suggestion",
			"autocomplete-first-suggestion",
			"autocomplete-last-suggestion",
			"autocomplete-prev-suggestion-page",
			"autocomplete-next-suggestion-page",
			"autocomplete-pick-suggestion",
			"autocomplete-pick-suggestion-alt",
			"autocomplete-pick-suggestion-alt-2",
			"autocomplete-update-suggestions",
			"autocomplete-close-signature-help",
			"autocomplete-prev-signature-help",
			"autocomplete-next-signature-help",
			"autocomplete-from-current-doc-symbols",
		};
		// clang-format on
		for ( const auto& key : list ) {
			if ( kb.contains( key ) ) {
				if ( !kb[key].empty() )
					mKeyBindings[key] = kb[key];
			} else {
				kb[key] = mKeyBindings[key];
				updateConfigFile = true;
			}
		}
	}

	if ( updateConfigFile ) {
		std::string newData = j.dump( 2 );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}

	if ( getUISceneNode() ) {
		updateShortcuts();
	}

	subscribeFileSystemListener();
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

void AutoCompletePlugin::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );
	std::vector<Uint32> listeners;
	listeners.push_back( editor->on( Event::OnDocumentLoaded, [this, editor]( const Event* ) {
		mDirty = true;
		mDocs.insert( editor->getDocumentRef().get() );
		mEditorDocs[editor] = editor->getDocumentRef().get();
		tryRequestCapabilities( editor );
	} ) );

	listeners.push_back( editor->on( Event::OnDocumentClosed, [this]( const Event* event ) {
		const DocEvent* docEvent = static_cast<const DocEvent*>( event );
		TextDocument* doc = docEvent->getDoc();

		{
			Lock ls( mDocUsesOwnSymbolsMutex );
			mDocUsesOwnSymbols.erase( doc );
		}

		Lock l( mDocMutex );
		mDocs.erase( doc );
		mDocCache.erase( doc );
		mDirty = true;
	} ) );

	listeners.push_back( editor->on( Event::OnDocumentChanged, [this, editor]( const Event* ) {
		TextDocument* oldDoc = mEditorDocs[editor];
		TextDocument* newDoc = editor->getDocumentRef().get();
		cancelSnippetSession( oldDoc );

		{
			Lock ls( mDocUsesOwnSymbolsMutex );
			mDocUsesOwnSymbols.erase( oldDoc );
		}

		Lock l( mDocMutex );
		mDocs.erase( oldDoc );
		mDocCache.erase( oldDoc );
		mEditorDocs[editor] = newDoc;
		mDirty = true;
	} ) );

	listeners.push_back( editor->on( Event::OnCursorPosChange, [this, editor]( const Event* ) {
		if ( !mReplacing )
			resetSuggestions( editor );

		if ( mSignatureHelpVisible && mSignatureHelpPosition.isValid() &&
			 !editor->getDocument().getSelection().hasSelection() &&
			 mSignatureHelpPosition.line() != editor->getDocument().getSelection().end().line() ) {
			resetSignatureHelp();
		}
	} ) );

	listeners.push_back( editor->on( Event::OnFocusLoss, [this, editor]( const Event* ) {
		resetSignatureHelp();
		cancelSnippetSession( &editor->getDocument(), true );
	} ) );

	listeners.push_back(
		editor->on( Event::OnDocumentUndoRedo, [this]( const Event* ) { resetSignatureHelp(); } ) );

	listeners.push_back(
		editor->on( Event::OnDocumentSyntaxDefinitionChange, [this]( const Event* ev ) {
			const DocSyntaxDefEvent* event = static_cast<const DocSyntaxDefEvent*>( ev );
			std::string oldLang = event->getOldLang();
			std::string newLang = event->getNewLang();
			mThreadPool->run( [this, oldLang, newLang] {
				updateLangCache( oldLang );
				updateLangCache( newLang );
			} );
		} ) );

	if ( editor->hasDocument() ) {
		editor->getDocument().setCommand(
			"autocomplete-from-current-doc-symbols", [this]( TextDocument::Client* client ) {
				Lock l( mDocUsesOwnSymbolsMutex );
				auto& usesOwnSymbols =
					mDocUsesOwnSymbols[&static_cast<UICodeEditor*>( client )->getDocument()];
				usesOwnSymbols = !usesOwnSymbols;
			} );
	}

	mEditors.insert( { editor, listeners } );
	mDocs.insert( editor->getDocumentRef().get() );
	mEditorDocs[editor] = editor->getDocumentRef().get();
	mDirty = true;
}

void AutoCompletePlugin::onUnregister( UICodeEditor* editor ) {
	if ( mShuttingDown )
		return;
	if ( mSuggestionsEditor == editor )
		resetSuggestions( editor );
	if ( mSignatureHelpEditor == editor )
		resetSignatureHelp();

	TextDocument* doc = nullptr;

	{
		Lock l( mDocMutex );
		doc = mEditorDocs[editor];
		auto cbs = mEditors[editor];
		for ( auto listener : cbs )
			editor->removeEventListener( listener );
		mEditors.erase( editor );
		mEditorDocs.erase( editor );
		for ( auto ceditor : mEditorDocs )
			if ( ceditor.second == doc )
				return;
		detachSnippetClient( doc );
		mDocs.erase( doc );
		mDocCache.erase( doc );
	}

	{
		Lock l( mDocUsesOwnSymbolsMutex );
		mDocUsesOwnSymbols.erase( doc );
	}

	mDirty = true;
}

static bool isSnippetNavigationCommand( std::string_view command ) {
	return command == "move-to-previous-char"sv || command == "move-to-next-char"sv ||
		   command == "move-to-previous-line"sv || command == "move-to-next-line"sv ||
		   command == "move-to-start-of-content"sv || command == "move-to-end-of-line"sv ||
		   command == "move-to-previous-page"sv || command == "move-to-next-page"sv;
}

bool AutoCompletePlugin::onKeyDown( UICodeEditor* editor, const KeyEvent& event ) {
	KeyBindings::Shortcut eventShortcut =
		KeyBindings::sanitizeShortcut( { event.getKeyCode(), event.getSanitizedMod() } );
	auto sessionIt = mSnippetSessions.find( &editor->getDocument() );
	if ( sessionIt != mSnippetSessions.end() && sessionIt->second.editor == editor ) {
		const Uint32 mod = event.getSanitizedMod();
		if ( event.getKeyCode() == KEY_TAB &&
			 ( mod == 0 || ( mod & KEYMOD_SHIFT && ( mod & ~KEYMOD_SHIFT ) == 0 ) ) ) {
			if ( mSnippetChoiceSuggestions && !( mod & KEYMOD_SHIFT ) )
				pickSnippetChoice( editor );
			return navigateSnippet( editor, mod & KEYMOD_SHIFT );
		}
		if ( event.getKeyCode() == KEY_ESCAPE && mod == 0 && mSnippetChoiceSuggestions ) {
			resetSuggestions( editor );
			editor->invalidateDraw();
			return true;
		}
		if ( event.getKeyCode() == KEY_ESCAPE && mod == 0 ) {
			cancelSnippetSession( &editor->getDocument() );
			return false;
		}
		const auto command = editor->getKeyBindings().getCommandFromKeyBind( eventShortcut );
		const bool choiceNavigationShortcut =
			mSnippetChoiceSuggestions &&
			( mShortcuts["autocomplete-prev-suggestion"] == eventShortcut ||
			  mShortcuts["autocomplete-next-suggestion"] == eventShortcut ||
			  mShortcuts["autocomplete-first-suggestion"] == eventShortcut ||
			  mShortcuts["autocomplete-last-suggestion"] == eventShortcut ||
			  mShortcuts["autocomplete-prev-suggestion-page"] == eventShortcut ||
			  mShortcuts["autocomplete-next-suggestion-page"] == eventShortcut );
		if ( !choiceNavigationShortcut && isSnippetNavigationCommand( command ) )
			cancelSnippetSession( &editor->getDocument(), true );
	}
	if ( mSignatureHelpVisible ) {
		if ( mShortcuts["autocomplete-close-signature-help"] == eventShortcut ) {
			resetSignatureHelp();
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-prev-signature-help"] == eventShortcut ) {
			if ( mSignatureHelp.signatures.size() > 1 ) {
				mSignatureHelpSelected = mSignatureHelpSelected == -1 ? 0 : mSignatureHelpSelected;
				++mSignatureHelpSelected;
				mSignatureHelpSelected =
					mSignatureHelpSelected % (int)mSignatureHelp.signatures.size();
				editor->invalidateDraw();
				return true;
			} else if ( mSuggestions.empty() ) {
				resetSignatureHelp();
			}
		} else if ( mShortcuts["autocomplete-next-signature-help"] == eventShortcut ) {
			if ( mSignatureHelp.signatures.size() > 1 ) {
				mSignatureHelpSelected = mSignatureHelpSelected <= 0
											 ? mSignatureHelp.signatures.size()
											 : mSignatureHelpSelected;
				--mSignatureHelpSelected;
				mSignatureHelpSelected = mSignatureHelpSelected % mSignatureHelp.signatures.size();
				editor->invalidateDraw();
				return true;
			} else if ( mSuggestions.empty() ) {
				resetSignatureHelp();
			}
		} else if ( event.getKeyCode() == KEY_BACKSPACE || event.getKeyCode() == KEY_DELETE ) {
			auto lang = editor->getDocumentRef()->getSyntaxDefinition().getLSPName();
			auto cap = mCapabilities.find( lang );
			if ( cap != mCapabilities.end() ) {
				auto curChar = event.getKeyCode() == KEY_BACKSPACE
								   ? editor->getDocumentRef()->getPrevChar()
								   : editor->getDocumentRef()->getCurrentChar();
				const auto& signatureTrigger = cap->second.signatureHelpProvider.triggerCharacters;
				if ( std::find( signatureTrigger.begin(), signatureTrigger.end(), curChar ) !=
					 signatureTrigger.end() ) {
					resetSignatureHelp();
				}
			}
		}
	}

	if ( !mSnippetChoiceSuggestions &&
		 mShortcuts["autocomplete-update-suggestions"] == eventShortcut ) {
		std::string partialSymbol( getPartialSymbol( &editor->getDocument() ) );
		updateSuggestions( partialSymbol, editor );
		return true;
	}

	if ( !mSuggestions.empty() ) {
		if ( mShortcuts["autocomplete-next-suggestion"] == eventShortcut ) {
			if ( mSuggestionIndex + 1 < (int)mSuggestions.size() ) {
				mSuggestionIndex++;
				if ( mSuggestionIndex < mSuggestionsStartIndex )
					mSuggestionsStartIndex = mSuggestionIndex;
				else if ( mSuggestionIndex > mSuggestionsStartIndex + mSuggestionsMaxVisible - 1 ) {
					mSuggestionsStartIndex =
						eemax( 0, mSuggestionIndex - ( mSuggestionsMaxVisible - 1 ) );
				}
			} else {
				mSuggestionIndex = 0;
				mSuggestionsStartIndex = 0;
			}
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-prev-suggestion"] == eventShortcut ) {
			if ( mSuggestionIndex - 1 < 0 ) {
				mSuggestionIndex = mSuggestions.size() - 1;
				mSuggestionsStartIndex =
					eemax( 0, (int)mSuggestions.size() - mSuggestionsMaxVisible );
			} else {
				mSuggestionIndex--;
			}
			if ( mSuggestionIndex < (int)mSuggestionsStartIndex )
				mSuggestionsStartIndex = mSuggestionIndex;
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-close-suggestion"] == eventShortcut ) {
			resetSuggestions( editor );
			resetSignatureHelp();
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-first-suggestion"] == eventShortcut ) {
			mSuggestionIndex = 0;
			mSuggestionsStartIndex = 0;
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-last-suggestion"] == eventShortcut ) {
			mSuggestionIndex = mSuggestions.size() - 1;
			mSuggestionsStartIndex = eemax( 0, (int)mSuggestions.size() - mSuggestionsMaxVisible );
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-prev-suggestion-page"] == eventShortcut ) {
			if ( mSuggestionIndex - (int)( mSuggestionsMaxVisible - 1 ) >= 0 ) {
				mSuggestionIndex -= ( mSuggestionsMaxVisible - 1 );
				if ( mSuggestionIndex < mSuggestionsStartIndex )
					mSuggestionsStartIndex = mSuggestionIndex;
			} else {
				mSuggestionIndex = 0;
				mSuggestionsStartIndex = 0;
			}
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-next-suggestion-page"] == eventShortcut ) {
			if ( mSuggestionIndex + mSuggestionsMaxVisible < (int)mSuggestions.size() ) {
				mSuggestionIndex += mSuggestionsMaxVisible - 1;
			} else {
				mSuggestionIndex = mSuggestions.size() - 1;
			}
			mSuggestionsStartIndex =
				eemax<int>( 0, mSuggestionIndex - ( mSuggestionsMaxVisible - 1 ) );
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts["autocomplete-pick-suggestion"] == eventShortcut ||
					mShortcuts["autocomplete-pick-suggestion-alt"] == eventShortcut ||
					mShortcuts["autocomplete-pick-suggestion-alt-2"] == eventShortcut ) {
			pickSuggestion( editor );
			return true;
		}
	}
	return false;
}

void AutoCompletePlugin::requestSignatureHelp( UICodeEditor* editor ) {
	{
		Lock l( mSignatureHelpEditorMutex );
		mSignatureHelpEditor = editor;
	}
	auto doc = editor->getDocumentRef();
	mSignatureHelpPosition = editor->getDocumentRef()->getSelection().start();

	mThreadPool->run( [this, editor]() {
		json data = getURIAndPositionJSON( editor );
		mManager->sendRequest( this, PluginMessageType::SignatureHelp, PluginMessageFormat::JSON,
							   &data );
	} );
}

void AutoCompletePlugin::requestCodeCompletion( UICodeEditor* editor ) {
	{
		Lock l( mHandlesMutex );
		auto handleIt = mHandles.find( editor->getDocumentRef().get() );
		if ( handleIt != mHandles.end() ) {
			for ( const PluginIDType& hndl : handleIt->second ) {
				auto data = getURIJSON( handleIt->first, hndl );
				mManager->sendBroadcast( PluginMessageType::CancelRequest,
										 PluginMessageFormat::JSON, &data );
			}
			handleIt->second.clear();
		}
	}
	json data = getURIAndPositionJSON( editor );
	PluginRequestHandle handle( mManager->sendRequest( this, PluginMessageType::CodeCompletion,
													   PluginMessageFormat::JSON, &data ) );
	Lock l( mHandlesMutex );
	mHandles[editor->getDocumentRef().get()].push_back( handle.id() );
}

bool AutoCompletePlugin::onTextInput( UICodeEditor* editor, const TextInputEvent& event ) {
	std::string partialSymbol( getPartialSymbol( &editor->getDocument() ) );

	auto lang = editor->getDocumentRef()->getSyntaxDefinition().getLSPName();
	auto cap = mCapabilities.find( lang );
	if ( cap != mCapabilities.end() ) {
		if ( cap->second.signatureHelpProvider.provider ) {
			bool requestedSignatureHelp = false;
			const auto& signatureTrigger = cap->second.signatureHelpProvider.triggerCharacters;
			if ( std::find( signatureTrigger.begin(), signatureTrigger.end(), event.getChar() ) !=
				 signatureTrigger.end() ) {
				requestSignatureHelp( editor );
				requestedSignatureHelp = true;
			}
			if ( mSignatureHelpVisible && !requestedSignatureHelp ) {
				auto doc = editor->getDocumentRef();
				auto curPos = doc->getSelection().start();
				if ( curPos.line() != mSignatureHelpPosition.line() ||
					 curPos < doc->startOfWord( doc->positionOffset( mSignatureHelpPosition, 1 ) ) )
					resetSignatureHelp();
			}
		}

		if ( cap->second.completionProvider.provider ) {
			const auto& triggerCharacters = cap->second.completionProvider.triggerCharacters;
			if ( partialSymbol.size() >= 1 ||
				 std::find( triggerCharacters.begin(), triggerCharacters.end(), event.getChar() ) !=
					 triggerCharacters.end() ) {
				updateSuggestions( partialSymbol, editor );
			} else {
				resetSuggestions( editor );
			}
		}
		return false;
	}

	if ( partialSymbol.size() >= 3 ) {
		updateSuggestions( partialSymbol, editor );
	} else {
		resetSuggestions( editor );
	}
	return false;
}

void AutoCompletePlugin::updateDocCache( TextDocument* doc ) {
	ScopedOp op(
		[this, doc] {
			Lock lu( mDocsUpdatingMutex );
			mDocsUpdating[doc] = true;
		},
		[this, doc] {
			Lock lu( mDocsUpdatingMutex );
			mDocsUpdating[doc] = false;
		} );

	Clock clock;
	std::unordered_map<TextDocument*, DocCache>::iterator docCache;
	{
		Lock l( mDocMutex );
		docCache = mDocCache.find( doc );
		if ( docCache == mDocCache.end() || mShuttingDown )
			return;
	}

	auto changeId = doc->getCurrentChangeId();
	auto symbols = getDocumentSymbols( doc );

	{
		Lock l( mDocMutex );
		docCache = mDocCache.find( doc );
		if ( docCache == mDocCache.end() || mShuttingDown )
			return;
		auto& cache = docCache->second;
		cache.changeId = changeId;
		cache.symbols = std::move( symbols );
	}

	std::string langName( doc->getSyntaxDefinition().getLanguageName() );
	{
		Lock l( mLangSymbolsMutex );
		auto& lang = mLangCache[langName];
		lang.clear();
		Lock l2( mDocMutex );
		for ( const auto& d : mDocCache ) {
			if ( d.first->getSyntaxDefinition().getLanguageName() == langName )
				lang.insert( lang.end(), d.second.symbols.begin(), d.second.symbols.end() );
		}
	}
	Log::debug( "Dictionary for %s updated in: %.2fms", doc->getFilename(),
				clock.getElapsedTime().asMilliseconds() );
}

void AutoCompletePlugin::updateLangCache( const std::string& langName ) {
	Clock clock;
	Lock l( mLangSymbolsMutex );
	Lock l2( mDocMutex );
	auto& lang = mLangCache[langName];
	lang.clear();
	for ( const auto& d : mDocCache ) {
		if ( d.first->getSyntaxDefinition().getLanguageName() == langName )
			lang.insert( lang.end(), d.second.symbols.begin(), d.second.symbols.end() );
	}
	Log::debug( "Lang dictionary for %s updated in: %.2fms", langName,
				clock.getElapsedTime().asMilliseconds() );
}

static SnippetParser::VariableMap snippetVariables( TextDocument& doc,
													const TextRange& selection ) {
	const TextPosition position = selection.normalized().start();
	std::string filePath = doc.getFilePath();
	if ( filePath.empty() )
		filePath = doc.getLoadingFilePath();
	const std::string filename = FileSystem::fileNameFromPath( filePath );
	return { { "TM_SELECTED_TEXT", doc.getText( selection ).toUtf8() },
			 { "TM_CURRENT_LINE", doc.getLineTextWithoutNewLine( position.line() ).toUtf8() },
			 { "TM_CURRENT_WORD", doc.getWordInPosition( position ).toUtf8() },
			 { "TM_LINE_INDEX", String::toString( position.line() ) },
			 { "TM_LINE_NUMBER", String::toString( position.line() + 1 ) },
			 { "TM_FILENAME", filename },
			 { "TM_FILENAME_BASE", FileSystem::fileRemoveExtension( filename ) },
			 { "TM_DIRECTORY", FileSystem::fileRemoveFileName( filePath ) },
			 { "TM_FILEPATH", filePath } };
}

void AutoCompletePlugin::pickSuggestion( UICodeEditor* editor ) {
	if ( mSnippetChoiceSuggestions )
		return pickSnippetChoice( editor );
	mReplacing = true;
	std::string symbol( getPartialSymbol( editor->getDocumentRef().get() ) );
	const auto& suggestion = mSuggestions[mSuggestionIndex];
	auto doc = editor->getDocumentRef();
	auto prevSels = doc->getSelections();
	const std::string& rawInsertText =
		!suggestion.insertText.empty() ? suggestion.insertText : suggestion.text;
	const bool isSnippet = suggestion.kind == LSPCompletionItemKind::Snippet ||
						   suggestion.insertTextFormat == LSPInsertTextFormat::Snippet;
	if ( !isSnippet ) {
		if ( doc->getSelections().size() == 1 && suggestion.range.isValid() &&
			 doc->isValidRange( suggestion.range ) ) {
			doc->setSelection( suggestion.range );
			doc->textInput( rawInsertText );
		} else {
			if ( !symbol.empty() )
				doc->execute( "delete-to-previous-word" );
			doc->textInput( rawInsertText );
		}
	} else {
		std::vector<SnippetInsertion> insertions;
		insertions.reserve( prevSels.size() );
		for ( const auto& selection : prevSels )
			insertions.push_back(
				{ SnippetParser::parse( rawInsertText, snippetVariables( *doc, selection ) ),
				  {} } );

		if ( prevSels.size() == 1 && suggestion.range.isValid() &&
			 doc->isValidRange( suggestion.range ) ) {
			doc->setSelection( suggestion.range );
		} else if ( !symbol.empty() ) {
			doc->execute( "delete-to-previous-word" );
		}
		if ( insertions.size() > doc->getSelections().size() )
			insertions.resize( doc->getSelections().size() );

		for ( size_t index = 0; index < insertions.size(); ++index ) {
			if ( doc->getSelectionIndex( index ).hasSelection() )
				doc->deleteTo( index, 0 );
			insertions[index].start = doc->getSelectionIndex( index ).start();
			TextPosition end = doc->insert( index, insertions[index].start,
											String::fromUtf8( insertions[index].snippet.text ) );
			doc->setSelection( index, end );
		}
		tryStartSnippetNav( insertions, editor );
	}

	mReplacing = false;
	resetSuggestions( editor );
	auto sessionIt = mSnippetSessions.find( doc.get() );
	if ( sessionIt != mSnippetSessions.end() )
		showSnippetChoices( sessionIt->second );
}

static TextPosition snippetPosition( const TextPosition& insertionStart,
									 const TextPosition& relative ) {
	return { insertionStart.line() + relative.line(),
			 relative.line() == 0 ? insertionStart.column() + relative.column()
								  : relative.column() };
}

void AutoCompletePlugin::tryStartSnippetNav( const std::vector<SnippetInsertion>& insertions,
											 UICodeEditor* editor ) {
	auto doc = editor->getDocumentRef();
	SnippetSession session;
	session.editor = editor;
	session.instanceCount = insertions.size();
	for ( size_t instance = 0; instance < insertions.size(); ++instance ) {
		const auto& insertion = insertions[instance];
		if ( !insertion.snippet.hasTabStops() )
			continue;
		String parsedText( String::fromUtf8( insertion.snippet.text ) );
		for ( const auto& stop : insertion.snippet.tabStops ) {
			auto groupIt = std::find_if(
				session.groups.begin(), session.groups.end(),
				[&stop]( const SnippetTabStopGroup& group ) { return group.index == stop.index; } );
			if ( groupIt == session.groups.end() ) {
				session.groups.push_back( { stop.index, {} } );
				groupIt = std::prev( session.groups.end() );
			}
			auto relativeRange =
				TextRange::convertToLineColumn( parsedText.view(), static_cast<Int64>( stop.start ),
												static_cast<Int64>( stop.end ) );
			if ( relativeRange.isValid() ) {
				groupIt->occurrences.push_back(
					{ { snippetPosition( insertion.start, relativeRange.start() ),
						snippetPosition( insertion.start, relativeRange.end() ) },
					  instance,
					  stop.choices } );
			}
		}
	}

	std::sort( session.groups.begin(), session.groups.end(),
			   []( const SnippetTabStopGroup& left, const SnippetTabStopGroup& right ) {
				   if ( left.index == 0 )
					   return false;
				   if ( right.index == 0 )
					   return true;
				   return left.index < right.index;
			   } );
	if ( session.groups.empty() )
		return;

	if ( session.groups.front().index == 0 ) {
		TextRanges finalStops;
		for ( const auto& occurrence : session.groups.front().occurrences )
			finalStops.push_back( occurrence.range );
		if ( !finalStops.empty() )
			doc->resetSelection( finalStops );
		return;
	}

	ensureSnippetClient( doc.get() );
	mSnippetSessions[doc.get()] = std::move( session );
	selectSnippetGroup( mSnippetSessions[doc.get()] );
}

static TextPosition snippetInsertedEnd( const TextPosition& start, const String& text ) {
	TextPosition end( start );
	for ( const auto& ch : text ) {
		if ( ch == '\n' ) {
			end.setLine( end.line() + 1 );
			end.setColumn( 0 );
		} else {
			end.setColumn( end.column() + 1 );
		}
	}
	return end;
}

static TextPosition translateSnippetSuffix( const TextPosition& position,
											const TextPosition& oldEnd,
											const TextPosition& newEnd ) {
	if ( position.line() == oldEnd.line() )
		return { newEnd.line(), newEnd.column() + position.column() - oldEnd.column() };
	return { position.line() + newEnd.line() - oldEnd.line(), position.column() };
}

static TextPosition translateSnippetMarker( const TextPosition& position, const TextRange& replaced,
											const TextPosition& insertedEnd, bool stickLeft ) {
	const auto& start = replaced.start();
	const auto& end = replaced.end();
	if ( position < start )
		return position;
	if ( position == start )
		return stickLeft ? start : insertedEnd;
	if ( position < end )
		return stickLeft ? start : insertedEnd;
	return translateSnippetSuffix( position, end, insertedEnd );
}

static void translateSnippetRange( TextRange& range, const TextRange& replaced,
								   const TextPosition& insertedEnd, bool changeInside ) {
	range.normalize();
	if ( changeInside ) {
		range.setStart( translateSnippetMarker( range.start(), replaced, insertedEnd, true ) );
		range.setEnd( translateSnippetMarker( range.end(), replaced, insertedEnd, false ) );
		return;
	}
	if ( range.end() <= replaced.start() )
		return;
	if ( range.start() >= replaced.end() ) {
		range.setStart( translateSnippetMarker( range.start(), replaced, insertedEnd, false ) );
		range.setEnd( translateSnippetMarker( range.end(), replaced, insertedEnd, false ) );
		return;
	}
	range.setStart( translateSnippetMarker( range.start(), replaced, insertedEnd, true ) );
	range.setEnd( translateSnippetMarker( range.end(), replaced, insertedEnd, false ) );
}

void AutoCompletePlugin::ensureSnippetClient( TextDocument* doc ) {
	if ( !doc || mSnippetClients.find( doc ) != mSnippetClients.end() )
		return;
	auto client = std::make_unique<SnippetDocumentClient>( this, doc );
	doc->registerClient( client.get() );
	mSnippetClients.emplace( doc, std::move( client ) );
}

void AutoCompletePlugin::detachSnippetClient( TextDocument* doc ) {
	cancelSnippetSession( doc );
	auto clientIt = mSnippetClients.find( doc );
	if ( clientIt == mSnippetClients.end() )
		return;
	clientIt->second->detach();
	mSnippetClients.erase( clientIt );
}

void AutoCompletePlugin::cancelSnippetSession( TextDocument* doc, bool collapseSelection ) {
	if ( !doc )
		return;
	auto sessionIt = mSnippetSessions.find( doc );
	if ( sessionIt == mSnippetSessions.end() )
		return;
	UICodeEditor* editor = sessionIt->second.editor;
	if ( collapseSelection && !doc->getSelections().empty() ) {
		TextRanges selections;
		const auto& session = sessionIt->second;
		if ( session.currentGroup < session.groups.size() ) {
			std::vector<bool> instanceAdded( session.instanceCount, false );
			for ( const auto& occurrence : session.groups[session.currentGroup].occurrences ) {
				if ( occurrence.instance < instanceAdded.size() &&
					 !instanceAdded[occurrence.instance] ) {
					selections.push_back( occurrence.range );
					instanceAdded[occurrence.instance] = true;
				}
			}
		}
		if ( selections.empty() )
			selections.push_back( doc->getSelection() );
		mChangingSnippetSelection = true;
		doc->resetSelection( selections );
		mChangingSnippetSelection = false;
	}
	mSnippetSessions.erase( sessionIt );
	if ( mSnippetChoiceSuggestions )
		resetSuggestions( editor );
	if ( editor )
		editor->invalidateDraw();
}

void AutoCompletePlugin::selectSnippetGroup( SnippetSession& session, bool showChoices ) {
	if ( session.currentGroup >= session.groups.size() )
		return;
	const auto& occurrences = session.groups[session.currentGroup].occurrences;
	if ( occurrences.empty() )
		return;
	TextRanges ranges;
	ranges.reserve( occurrences.size() );
	for ( const auto& occurrence : occurrences )
		ranges.push_back( occurrence.range );
	ranges.sort();
	mChangingSnippetSelection = true;
	session.editor->getDocument().resetSelection( ranges );
	mChangingSnippetSelection = false;
	session.editor->invalidateDraw();
	if ( showChoices )
		showSnippetChoices( session );
}

void AutoCompletePlugin::showSnippetChoices( SnippetSession& session ) {
	resetSuggestions( session.editor );
	if ( session.currentGroup >= session.groups.size() )
		return;
	const std::vector<std::string>* choices = nullptr;
	for ( const auto& occurrence : session.groups[session.currentGroup].occurrences ) {
		if ( !occurrence.choices.empty() ) {
			choices = &occurrence.choices;
			break;
		}
	}
	if ( !choices )
		return;
	Lock suggestionsLock( mSuggestionsMutex );
	mSuggestions.reserve( choices->size() );
	for ( const auto& choice : *choices )
		mSuggestions.emplace_back( choice );
	{
		Lock editorLock( mSuggestionsEditorMutex );
		mSuggestionsEditor = session.editor;
	}
	mSnippetChoiceSuggestions = true;
	session.editor->invalidateDraw();
}

void AutoCompletePlugin::pickSnippetChoice( UICodeEditor* editor ) {
	if ( mSuggestionIndex < 0 || mSuggestionIndex >= static_cast<int>( mSuggestions.size() ) )
		return;
	const std::string choice = mSuggestions[mSuggestionIndex].text;
	TextDocument* doc = &editor->getDocument();
	auto sessionIt = mSnippetSessions.find( doc );
	if ( sessionIt == mSnippetSessions.end() || sessionIt->second.editor != editor ) {
		resetSuggestions( editor );
		return;
	}
	doc->textInput( String::fromUtf8( choice ) );
	resetSuggestions( editor );
	sessionIt = mSnippetSessions.find( doc );
	if ( sessionIt != mSnippetSessions.end() )
		selectSnippetGroup( sessionIt->second, false );
}

bool AutoCompletePlugin::navigateSnippet( UICodeEditor* editor, bool backwards ) {
	TextDocument* doc = &editor->getDocument();
	auto sessionIt = mSnippetSessions.find( doc );
	if ( sessionIt == mSnippetSessions.end() || sessionIt->second.editor != editor )
		return false;
	auto& session = sessionIt->second;
	if ( backwards ) {
		if ( session.currentGroup == 0 )
			return true;
		--session.currentGroup;
		selectSnippetGroup( session );
		return true;
	}

	if ( session.currentGroup + 1 >= session.groups.size() ) {
		cancelSnippetSession( doc, true );
		return true;
	}
	++session.currentGroup;
	if ( session.groups[session.currentGroup].index != 0 ) {
		selectSnippetGroup( session );
		return true;
	}

	TextRanges finalStops;
	for ( const auto& occurrence : session.groups[session.currentGroup].occurrences )
		finalStops.push_back( occurrence.range );
	if ( mSnippetChoiceSuggestions )
		resetSuggestions( editor );
	mSnippetSessions.erase( sessionIt );
	if ( !finalStops.empty() ) {
		mChangingSnippetSelection = true;
		doc->resetSelection( finalStops );
		mChangingSnippetSelection = false;
	}
	editor->invalidateDraw();
	return true;
}

void AutoCompletePlugin::onSnippetTextChanged( TextDocument* doc,
											   const DocumentContentChange& change ) {
	auto sessionIt = mSnippetSessions.find( doc );
	if ( sessionIt == mSnippetSessions.end() )
		return;
	auto& session = sessionIt->second;
	if ( session.currentGroup >= session.groups.size() ) {
		cancelSnippetSession( doc );
		return;
	}

	TextRange replaced( change.range.normalized() );
	const auto& activeOccurrences = session.groups[session.currentGroup].occurrences;
	auto editedOccurrence = std::find_if(
		activeOccurrences.begin(), activeOccurrences.end(), [&replaced]( const auto& occurrence ) {
			return occurrence.range.normalized().contains( replaced );
		} );
	if ( editedOccurrence == activeOccurrences.end() ) {
		cancelSnippetSession( doc );
		return;
	}

	const TextRange activeRange( editedOccurrence->range.normalized() );
	const TextPosition insertedEnd = snippetInsertedEnd( replaced.start(), change.text );
	for ( auto& group : session.groups ) {
		for ( auto& occurrence : group.occurrences ) {
			const bool changeInside = occurrence.range.normalized().contains( activeRange );
			translateSnippetRange( occurrence.range, replaced, insertedEnd, changeInside );
		}
	}
	if ( mSnippetChoiceSuggestions )
		resetSuggestions( session.editor );
	if ( session.editor )
		session.editor->invalidateDraw();
}

void AutoCompletePlugin::onSnippetSelectionChanged( TextDocument* doc ) {
	if ( mChangingSnippetSelection || doc->isDoingTextInput() )
		return;
	auto sessionIt = mSnippetSessions.find( doc );
	if ( sessionIt == mSnippetSessions.end() )
		return;
	const auto& session = sessionIt->second;
	if ( session.currentGroup >= session.groups.size() )
		return cancelSnippetSession( doc );
	const auto& activeOccurrences = session.groups[session.currentGroup].occurrences;
	const auto& selections = doc->getSelections();
	if ( selections.size() != activeOccurrences.size() )
		return cancelSnippetSession( doc );
	for ( const auto& selection : selections ) {
		if ( std::none_of( activeOccurrences.begin(), activeOccurrences.end(),
						   [&selection]( const auto& occurrence ) {
							   return occurrence.range.normalized().contains(
								   selection.normalized() );
						   } ) ) {
			cancelSnippetSession( doc );
			return;
		}
	}
}

void AutoCompletePlugin::onSnippetDocumentClosed( TextDocument* doc ) {
	cancelSnippetSession( doc );
}

PluginRequestHandle
AutoCompletePlugin::processCodeCompletion( const LSPCompletionList& completion ) {
	SymbolsList suggestions;
	// FIX: Find a way of passing some messages as non-const and allow to move them.
	// Creating a copy of each element is unnecessary and expensive, this time we are going to
	// hack it and remove the constness of the suggestions to be able to move all its internal
	// values
	LSPCompletionList& wcompletion = const_cast<LSPCompletionList&>( completion );
	for ( auto& item : wcompletion.items ) {
		if ( !item.textEdit.text.empty() ) {
			suggestions.push_back( { item.kind,
									 std::move( item.label.empty() ? item.insertText : item.label ),
									 std::move( item.detail ), std::move( item.sortText ),
									 item.textEdit.range, std::move( item.textEdit.text ),
									 std::move( item.documentation ), item.insertTextFormat } );
		} else if ( !item.insertText.empty() ) {
			suggestions.push_back( { item.kind,
									 std::move( item.label.empty() ? item.insertText : item.label ),
									 std::move( item.detail ), std::move( item.sortText ),
									 item.textEdit.range, std::string{ item.insertText },
									 std::move( item.documentation ), item.insertTextFormat } );
		} else {
			suggestions.push_back( { item.kind,
									 std::move( item.filterText ),
									 std::move( item.detail ),
									 std::move( item.sortText ),
									 {},
									 "",
									 std::move( item.documentation ),
									 item.insertTextFormat } );
		}
	}
	if ( suggestions.empty() || !mSuggestionsEditor )
		return {};
	UICodeEditor* editor = nullptr;
	{
		Lock l( mSuggestionsEditorMutex );
		editor = mSuggestionsEditor;
	}
	if ( !editor )
		return {};
	std::string symbol( getPartialSymbol( editor->getDocumentRef().get() ) );
	const std::string& lang = editor->getDocument().getSyntaxDefinition().getLanguageName();
	bool hasLangSuggestions = false;
	{
		Lock l2( mLangSymbolsMutex );
		auto langSuggestions = mLangCache.find( lang );
		hasLangSuggestions = langSuggestions != mLangCache.end();
	}
	if ( symbol.empty() || !hasLangSuggestions ) {
		Lock l( mSuggestionsMutex );
		mSuggestions = suggestions;
	} else {
		SymbolsList fuzzySuggestions;
		{
			Lock l2( mLangSymbolsMutex );
			auto& symbols = mLangCache[lang];
			fuzzySuggestions = fuzzyMatchSymbols( { &suggestions, &symbols }, symbol,
												  eemax<size_t>( 100UL, suggestions.size() ) );
		}

		if ( fuzzySuggestions.empty() && !suggestions.empty() ) {
			for ( const auto& suggestion : suggestions )
				if ( String::startsWith( suggestion.text, symbol ) )
					fuzzySuggestions.emplace_back( std::move( suggestion ) );
		}

		Lock l( mSuggestionsMutex );
		mSuggestions = fuzzySuggestions;
	}

	editor->runOnMainThread( [editor] { editor->invalidateDraw(); } );

	return {};
}

PluginRequestHandle
AutoCompletePlugin::processSignatureHelp( const LSPSignatureHelp& signatureHelp ) {
	UICodeEditor* editor = nullptr;
	{
		Lock l( mSignatureHelpEditorMutex );
		editor = mSignatureHelpEditor;
	}
	if ( !editor )
		return {};

	// Convert the LSP Signature Help into our own object:
	// We will convert the UTF-8 label to UTF-32, then we will remove any new lines and extra spaces
	// This guarantees that we always display a single line signature help when requested (this is
	// optional)
	SignatureHelp signatures;
	signatures.activeSignature = signatureHelp.activeSignature;
	signatures.activeParameter = signatureHelp.activeParameter;
	signatures.signatures.reserve( signatureHelp.signatures.size() );

	TextDocument doc;

	for ( size_t sigIdx = 0; sigIdx < signatureHelp.signatures.size(); sigIdx++ ) {
		const auto& sig = signatureHelp.signatures[sigIdx];

		String initialLabel( sig.label );
		SignatureInformation nsig;
		nsig.documentation = sig.documentation;

		doc.reset();
		doc.textInput( initialLabel );

		std::vector<String> parameters;
		nsig.parameters.reserve( sig.parameters.size() );

		if ( !mSignatureHelpMultiLine )
			parameters.reserve( sig.parameters.size() );

		Int32 skippedBeforeActiveParameter = 0;

		for ( size_t i = 0; i < sig.parameters.size(); i++ ) {
			const auto rawStart = sig.parameters[i].start;
			const auto rawEnd = sig.parameters[i].end;

			const bool isBeforeActiveParameter =
				static_cast<Int32>( i ) < signatureHelp.activeParameter;

			if ( rawStart < 0 || rawEnd < 0 || rawEnd < rawStart ||
				 static_cast<size_t>( rawEnd ) > sig.label.size() ) {
				if ( sigIdx == static_cast<size_t>( signatureHelp.activeSignature ) &&
					 isBeforeActiveParameter ) {
					skippedBeforeActiveParameter++;
				}
				continue;
			}

			auto start = String::utf8ToCodepointPosition( sig.label, rawStart );
			auto end = String::utf8ToCodepointPosition( sig.label, rawEnd );

			if ( start < 0 || end < 0 || end < start ) {
				if ( sigIdx == static_cast<size_t>( signatureHelp.activeSignature ) &&
					 isBeforeActiveParameter ) {
					skippedBeforeActiveParameter++;
				}
				continue;
			}

			auto sel = TextRange::convertToLineColumn( initialLabel.view(), start, end );

			nsig.parameters.emplace_back(
				TextSelectionRange{ static_cast<Int64>( start ), static_cast<Int64>( end ) } );

			if ( !mSignatureHelpMultiLine )
				parameters.emplace_back( doc.getText( sel ) );
		}

		if ( sigIdx == static_cast<size_t>( signatureHelp.activeSignature ) ) {
			signatures.activeParameter =
				eemax<Int32>( 0, signatures.activeParameter - skippedBeforeActiveParameter );
		}

		if ( !mSignatureHelpMultiLine && 0 != doc.replaceAll( "\n", "" ) ) {
			while ( 0 != doc.replaceAll( "  ", " " ) )
				;

			nsig.label = doc.getLineTextWithoutNewLine( 0 );
			nsig.parameters.clear();

			for ( const auto& param : parameters ) {
				auto res = doc.find( param );
				if ( res.isValid() ) {
					nsig.parameters.push_back(
						TextRange::convertToOffset( nsig.label.view(), res.result ) );
				}
			}
		} else {
			nsig.label = std::move( initialLabel );
		}

		signatures.signatures.emplace_back( std::move( nsig ) );
	}

	if ( signatures.signatures.empty() ) {
		signatures.activeSignature = 0;
		signatures.activeParameter = 0;
	} else {
		signatures.activeSignature =
			eemin<Int32>( eemax<Int32>( signatures.activeSignature, 0 ),
						  static_cast<Int32>( signatures.signatures.size() ) - 1 );

		const auto& activeSig = signatures.signatures[signatures.activeSignature];

		if ( activeSig.parameters.empty() ) {
			signatures.activeParameter = 0;
		} else {
			signatures.activeParameter =
				eemin<Int32>( eemax<Int32>( signatures.activeParameter, 0 ),
							  static_cast<Int32>( activeSig.parameters.size() ) - 1 );
		}
	}

	editor->runOnMainThread( [this, editor, signatures = std::move( signatures )] {
		mSignatureHelpVisible = true;
		mSignatureHelp = signatures;

		if ( mSignatureHelpSelected >= static_cast<Int32>( mSignatureHelp.signatures.size() ) )
			mSignatureHelpSelected = -1;

		if ( mSignatureHelp.signatures.empty() )
			resetSignatureHelp();

		editor->invalidateDraw();
	} );

	return {};
}

void AutoCompletePlugin::updateShortcuts() {
	const auto toShortcut = [this]( const std::string& keys ) {
		return KeyBindings::toShortcut(
			getManager()->getUISceneNode()->getEventDispatcher()->getInput(), keys );
	};

	for ( const auto& kb : mKeyBindings )
		mShortcuts[kb.first] = toShortcut( kb.second );
}

PluginRequestHandle AutoCompletePlugin::processResponse( const PluginMessage& msg ) {
	if ( msg.type == PluginMessageType::UIReady ) {
		updateShortcuts();
	} else if ( msg.isResponse() && msg.type == PluginMessageType::CodeCompletion ) {
		if ( msg.responseID ) {
			Lock l( mHandlesMutex );
			for ( auto& handle : mHandles ) {
				auto find = std::find( handle.second.begin(), handle.second.end(), msg.responseID );
				if ( find != handle.second.end() )
					handle.second.erase( find );
			}
		}
		return processCodeCompletion( msg.asCodeCompletion() );
	} else if ( msg.isRequest() && msg.type == PluginMessageType::SignatureHelp ) {
		if ( getManager() && getManager()->getSplitter() &&
			 getManager()->getSplitter()->curEditorIsNotNull() ) {
			requestSignatureHelp( getManager()->getSplitter()->getCurEditor() );
		}
	} else if ( msg.isResponse() && msg.type == PluginMessageType::SignatureHelp ) {
		return processSignatureHelp( msg.asSignatureHelp() );
	} else if ( msg.isBroadcast() && msg.type == PluginMessageType::LanguageServerCapabilities ) {
		if ( msg.asLanguageServerCapabilities().ready ) {
			LSPServerCapabilities cap = msg.asLanguageServerCapabilities();
			auto& trig = cap.signatureHelpProvider.triggerCharacters;
			static const std::vector<std::pair<char, char>> pairs = {
				{ '(', ')' }, { '{', '}' }, { '<', '>' } };
			for ( const auto& pair : pairs ) {
				if ( std::find( trig.begin(), trig.end(), pair.first ) != trig.end() &&
					 std::find( trig.begin(), trig.end(), pair.second ) == trig.end() ) {
					trig.push_back( pair.second );
				}
			}
			Lock l( mCapabilitiesMutex );
			for ( const auto& lang : cap.languages )
				mCapabilities[lang] = cap;
		}
	}
	return {};
}

bool AutoCompletePlugin::tryRequestCapabilities( UICodeEditor* editor ) {
	const auto& language = editor->getDocumentRef()->getSyntaxDefinition().getLSPName();
	auto it = mCapabilities.find( language );
	if ( it != mCapabilities.end() )
		return true;
	json data;
	data["language"] = language;
	mManager->sendRequest( this, PluginMessageType::LanguageServerCapabilities,
						   PluginMessageFormat::JSON, &data );
	return false;
}

std::string AutoCompletePlugin::getPartialSymbol( TextDocument* doc ) {
	TextPosition end = doc->getSelection().end();
	TextPosition start = doc->startOfWord( end );
	return doc->getText( { start, end } ).toUtf8();
}

void AutoCompletePlugin::update( UICodeEditor* editor ) {
	const int iconSize = PixelDensity::dpToPxI( 12 );
	if ( mSuggestionIconDrawableSize != iconSize ) {
		mSuggestionIconDrawables.clear();
		mSuggestionIconDrawableSize = iconSize;
	}
	for ( const auto& suggestion : mSuggestions ) {
		const int iconKind = (int)suggestion.kind;
		if ( mSuggestionIconDrawables.find( iconKind ) != mSuggestionIconDrawables.end() )
			continue;
		UIIcon* icon = editor->getUISceneNode()->findIcon(
			LSPCompletionItemHelper::toIconString( suggestion.kind ) );
		if ( icon )
			mSuggestionIconDrawables[iconKind] = icon->createDrawable( iconSize );
	}

	for ( auto clientIt = mSnippetClients.begin(); clientIt != mSnippetClients.end(); ) {
		if ( !clientIt->second->isAttached() )
			clientIt = mSnippetClients.erase( clientIt );
		else
			++clientIt;
	}
	if ( mClock.getElapsedTime() >= mUpdateFreq || mDirty ) {
		mClock.restart();
		mDirty = false;
		Lock l( mDocMutex );
		for ( auto& doc : mDocs ) {
			if ( !doc->isLoading() && mDocCache[doc].changeId != doc->getCurrentChangeId() ) {
				{
					Lock lu( mDocsUpdatingMutex );
					auto du = mDocsUpdating.find( doc );
					// Dont update the document cache if it's still updating the document
					if ( du != mDocsUpdating.end() && du->second == true )
						continue;
				}
				mThreadPool->run( [this, doc] { updateDocCache( doc ); } );
			}
		}
	}
}

void AutoCompletePlugin::drawSignatureHelp( UICodeEditor* editor, const Vector2f& startScroll,
											const Float& lineHeight, bool drawUp ) {
	TextDocument& doc = editor->getDocument();
	Primitives primitives;
	const SyntaxColorScheme& scheme = editor->getColorScheme();
	const auto& normalStyle = scheme.getEditorSyntaxStyle( "suggestion"_sst );
	const auto& selectedStyle = scheme.getEditorSyntaxStyle( "suggestion_selected"_sst );
	const auto& matchingSelection = scheme.getEditorSyntaxStyle( "matching_selection"_sst );

	auto curSigIdx =
		mSignatureHelpSelected != -1 ? mSignatureHelpSelected : mSignatureHelp.activeSignature;
	if ( curSigIdx >= (int)mSignatureHelp.signatures.size() )
		return;
	auto curSig = mSignatureHelp.signatures[curSigIdx];
	primitives.setColor( Color( selectedStyle.background ).blendAlpha( editor->getAlpha() ) );
	String str;
	if ( mSignatureHelp.signatures.size() > 1 ) {
		str = String::format( "%s (%d of %zu)", curSig.label.toUtf8(),
							  mSignatureHelpSelected == -1 ? 1 : mSignatureHelpSelected + 1,
							  mSignatureHelp.signatures.size() );
	} else {
		str = curSig.label;
	}

	mSignatureHelpText.setFont( editor->getFont() );
	mSignatureHelpText.setFontSize( editor->getFontSize() );
	mSignatureHelpText.setFillColor( normalStyle.color );
	mSignatureHelpText.setStyle( normalStyle.style );
	if ( mSignatureHelpText.setString( str ) ) {
		SyntaxTokenizer::tokenizeText( doc.getSyntaxDefinition(), editor->getColorScheme(),
									   &mSignatureHelpText );
	}

	if ( mSignatureHelpMultiLine ) {
		mSignatureHelpText.setLineWrapMode( LineWrapMode::Word );
		mSignatureHelpText.setLineWrapKeepIndentation( true );
		mSignatureHelpText.setMaxWrapWidth( editor->convertLength(
			StyleSheetLength( mMaxSignatureHelperWidth ), editor->getPixelsSize().getWidth() ) );
	}

	Float boxWidth = mSignatureHelpText.getTextWidth() + mBoxPadding.Left + mBoxPadding.Right;
	Float boxHeight =
		mSignatureHelpText.getVisualLineCount() * mSignatureHelpText.getLineSpacing() +
		mBoxPadding.Top + mBoxPadding.Bottom;

	Float vdiff = drawUp ? -boxHeight : mRowHeight;
	auto offset = editor->getTextPositionOffset( mSignatureHelpPosition ).asFloat();

	Vector2f pos( startScroll.x + offset.x, startScroll.y + offset.y + vdiff );
	Rectf boxRect( pos, Sizef( boxWidth, boxHeight ) );

	Float screenRight = editor->getScreenPos().x + editor->getPixelsSize().getWidth();
	if ( boxRect.Right > screenRight ) {
		boxRect.setPosition( { eefloor( screenRight - boxWidth ), boxRect.getPosition().y } );
		if ( boxRect.getPosition().x < editor->getScreenPos().x )
			boxRect.setPosition( { eefloor( editor->getScreenPos().x ), boxRect.getPosition().y } );
	}

	bool hasParams = !curSig.parameters.empty();

	auto curParam =
		hasParams ? curSig.parameters[mSignatureHelp.activeParameter % curSig.parameters.size()]
				  : TextSelectionRange{};

	SmallVector<Rectf> paramRects;
	if ( hasParams ) {
		paramRects = mSignatureHelpText.getSelectionRects( curParam );

		if ( !paramRects.empty() ) {
			for ( auto& r : paramRects )
				r.move( { boxRect.Left + mBoxPadding.Left, boxRect.Top + mBoxPadding.Top } );

			if ( !mSignatureHelpMultiLine && !editor->getScreenRect().contains( paramRects[0] ) ) {
				paramRects[0].move(
					{ -( boxRect.Left + mBoxPadding.Left ), -( boxRect.Top + mBoxPadding.Top ) } );
				pos = { static_cast<Float>( startScroll.x + offset.x - paramRects[0].Left ),
						static_cast<Float>( startScroll.y + offset.y + vdiff ) };
				boxRect.setPosition( pos );
				paramRects[0].setPosition( { boxRect.Left + mBoxPadding.Left + paramRects[0].Left,
											 boxRect.getPosition().y } );
			}
		}
	}

	primitives.drawRoundedRectangle( boxRect, 0.f, Vector2f::One, 6 );

	primitives.setColor( matchingSelection.color );
	for ( const auto& rect : paramRects )
		primitives.drawRoundedRectangle( rect, 0.f, Vector2f::One, 6 );

	mSignatureHelpText.draw( boxRect.getPosition().x + mBoxPadding.Left,
							 boxRect.getPosition().y + mBoxPadding.Top );

	bool drawsSuggestions =
		!( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor );

	if ( !drawsSuggestions && mSignatureHelpDocumentation && !curSig.documentation.value.empty() ) {
		mSuggestionDoc.setFillColor( normalStyle.color );
		mSuggestionDoc.setStyle( normalStyle.style );
		mSuggestionDoc.setFont( editor->getFont() );
		mSuggestionDoc.setFontSize( editor->getFontSize() );
		mSuggestionDoc.setLineWrapMode( LineWrapMode::Word );
		mSuggestionDoc.setLineWrapKeepIndentation( true );

		Vector2f cursorScreenPos( startScroll.x + offset.x, startScroll.y + offset.y );
		Rectf docRect =
			findBestDocumentationPlacement( editor, curSig.documentation, "", boxRect, boxRect,
											cursorScreenPos, drawUp, lineHeight );

		if ( docRect.getSize().getWidth() > 0 && docRect.getSize().getHeight() > 0 ) {
			primitives.setColor(
				Color( selectedStyle.background ).blendAlpha( editor->getAlpha() ) );

			editor->clipSmartEnable( docRect.Left, docRect.Top, docRect.getWidth(),
									 docRect.getHeight() );

			primitives.drawRoundedRectangle( docRect, 0.f, Vector2f::One, 6 );

			mSuggestionDoc.draw( docRect.Left + mBoxPadding.Left, docRect.Top + mBoxPadding.Top );

			editor->clipSmartDisable();
		}
	}
}

void AutoCompletePlugin::postDraw( UICodeEditor* editor, const Vector2f& startScroll,
								   const Float& lineHeight, const TextPosition& cursor ) {
	bool drawsSuggestions =
		!( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor );
	bool drawsSignature = mSignatureHelpVisible && mSignatureHelpEditor == editor &&
						  !mSignatureHelp.signatures.empty() && mSignatureHelpPosition.isValid();
	if ( !drawsSuggestions && !drawsSignature )
		return;

	TextDocument& doc = editor->getDocument();
	TextPosition start = doc.startOfWord( editor->getDocument().startOfWord( cursor ) );
	Primitives primitives;
	const SyntaxColorScheme& scheme = editor->getColorScheme();
	const auto& normalStyle = scheme.getEditorSyntaxStyle( "suggestion"_sst );
	const auto& selectedStyle = scheme.getEditorSyntaxStyle( "suggestion_selected"_sst );
	bool drawUp = true;
	mRowHeight = lineHeight + mBoxPadding.Top + mBoxPadding.Bottom;

	if ( !drawsSuggestions ) {
		if ( drawsSignature )
			drawSignatureHelp( editor, startScroll, lineHeight, drawUp );
		return;
	}

	SymbolsList suggestions;
	{
		Lock l( mSuggestionsMutex );
		suggestions = mSuggestions;
	}

	auto offset = editor->getTextPositionOffset( start );
	Vector2f cursorPos( startScroll.x + offset.x, startScroll.y + offset.y + lineHeight );
	size_t largestString = 0;
	size_t max = eemin<size_t>( mSuggestionsMaxVisible, suggestions.size() );

	const auto& barStyle = scheme.getEditorSyntaxStyle( "suggestion_scrollbar"_sst );
	if ( cursorPos.y + mRowHeight * max > editor->getPixelsSize().getHeight() ) {
		cursorPos.y -= lineHeight + mRowHeight * max;
		drawUp = false;
	}

	size_t maxIndex =
		eemin<size_t>( mSuggestionsStartIndex + mSuggestionsMaxVisible, suggestions.size() );

	std::vector<String> visibleStrings;
	size_t visibleStrIndex = 0;
	visibleStrings.resize( maxIndex - mSuggestionsStartIndex );
	for ( size_t i = mSuggestionsStartIndex; i < maxIndex; i++ ) {
		bool needsEllipsis = suggestions[i].text.size() > mMaxLabelCharacters;
		String str{ needsEllipsis ? suggestions[i].text.substr( 0, mMaxLabelCharacters )
								  : suggestions[i].text };
		if ( needsEllipsis )
			str[str.size() - 1] = 0x2026 /* u'…' */;
		auto nlPos = str.find_first_of( '\n' );
		if ( nlPos == String::InvalidPos )
			str = str.substr( 0, nlPos );
		String::trimInPlace( str );
		largestString = eemax<size_t>( largestString, editor->getTextWidth( str ) );
		visibleStrings[visibleStrIndex] = std::move( str );
		visibleStrIndex++;
	}

	Sizef bar( PixelDensity::dpToPxI( 6 ),
			   eemax( PixelDensity::dpToPx( 8 ),
					  mRowHeight * max * ( mSuggestionsMaxVisible / (Float)suggestions.size() ) ) );
	Sizef iconSpace( PixelDensity::dpToPxI( 16 ), mRowHeight );
	mBoxRect = Rectf( Vector2f( cursorPos.x, cursorPos.y ) - editor->getScreenPos(),
					  Sizef( largestString + mBoxPadding.Left + mBoxPadding.Right +
								 iconSpace.getWidth() + bar.getWidth(),
							 mRowHeight * max ) );

	size_t count = 0;
	Rectf boxRect( { mBoxRect.getPosition() + editor->getScreenPos(), mBoxRect.getSize() } );
	primitives.setColor( Color( normalStyle.background ).blendAlpha( editor->getAlpha() ) );
	primitives.drawRoundedRectangle( boxRect, 0.f, Vector2f::One, 6 );

	visibleStrIndex = 0;
	for ( size_t i = mSuggestionsStartIndex; i < maxIndex; i++ ) {
		const auto& suggestion = suggestions[i];

		if ( mSuggestionIndex == (int)i ) {
			primitives.setColor(
				Color( selectedStyle.background ).blendAlpha( editor->getAlpha() ) );
			primitives.drawRoundedRectangle(
				Rectf( Vector2f( cursorPos.x, cursorPos.y + mRowHeight * count ),
					   Sizef( mBoxRect.getWidth(), mRowHeight ) ),
				0.f, Vector2f::One, 6 );
		}
		Text text( "", editor->getFont(), editor->getFontSize() );
		text.setFillColor( mSuggestionIndex == (int)i ? selectedStyle.color : normalStyle.color );
		text.setStyle( mSuggestionIndex == (int)i ? selectedStyle.style : normalStyle.style );
		text.setString( visibleStrings[visibleStrIndex] );

		if ( mHighlightSuggestions && suggestion.kind != LSPCompletionItemKind::Text ) {
			SyntaxTokenizer::tokenizeText( doc.getSyntaxDefinition(), editor->getColorScheme(),
										   &text );
		}

		text.draw( cursorPos.x + iconSpace.getWidth() + mBoxPadding.Left,
				   cursorPos.y + mRowHeight * count + mBoxPadding.Top );

		auto iconIt = mSuggestionIconDrawables.find( (int)suggestion.kind );
		DrawablePtr icon = iconIt != mSuggestionIconDrawables.end() ? iconIt->second : DrawablePtr{};

		if ( icon ) {
			Color iconColor( icon->getColor() );
			icon->setColor( mSuggestionIndex == (int)i ? selectedStyle.color : normalStyle.color );
			Vector2f padding(
				eefloor( ( iconSpace.getWidth() - icon->getSize().getWidth() ) * 0.5f ),
				eefloor( ( iconSpace.getHeight() - icon->getSize().getHeight() ) * 0.5f ) );
			icon->draw( { cursorPos.x + padding.x, cursorPos.y + mRowHeight * count + padding.y } );
			icon->setColor( iconColor );
		}

		if ( mSuggestionDocumentation && mSuggestionIndex == (int)i &&
			 !suggestion.documentation.value.empty() ) {
			mSuggestionDoc.setFillColor( normalStyle.color );
			mSuggestionDoc.setStyle( normalStyle.style );
			mSuggestionDoc.setFont( editor->getFont() );
			mSuggestionDoc.setFontSize( editor->getFontSize() );
			mSuggestionDoc.setLineWrapMode( LineWrapMode::Word );
			mSuggestionDoc.setLineWrapKeepIndentation( true );

			Vector2f cursorOffset = editor->getTextPositionOffset( cursor ).asFloat();
			Vector2f cursorScreenPos( startScroll.x + cursorOffset.x,
									  startScroll.y + cursorOffset.y );
			Rectf docRect = findBestDocumentationPlacement(
				editor, suggestion.documentation, suggestion.detail, boxRect,
				{ { cursorPos.x, cursorPos.y + mRowHeight * count },
				  { mBoxRect.getWidth(), mRowHeight } },
				cursorScreenPos, drawUp, lineHeight );

			if ( docRect.getSize().getWidth() > 0 && docRect.getSize().getHeight() > 0 ) {
				primitives.setColor(
					Color( selectedStyle.background ).blendAlpha( editor->getAlpha() ) );

				editor->clipSmartEnable( docRect.Left, docRect.Top, docRect.getWidth(),
										 docRect.getHeight() );

				primitives.drawRoundedRectangle( docRect, 0.f, Vector2f::One, 6 );

				mSuggestionDoc.draw( docRect.Left + mBoxPadding.Left,
									 docRect.Top + mBoxPadding.Top );

				editor->clipSmartDisable();
			}
		}
		count++;
		visibleStrIndex++;
	}

	if ( drawsSignature )
		drawSignatureHelp( editor, startScroll, lineHeight, drawUp );

	if ( max >= suggestions.size() )
		return;

	primitives.setColor( barStyle.color );
	Float yPos =
		mSuggestionsStartIndex > 0
			? mSuggestionsStartIndex / (Float)( suggestions.size() - mSuggestionsMaxVisible )
			: 0;
	Rectf barRect( { Vector2f( cursorPos.x + mBoxRect.getWidth() - bar.getWidth(),
							   cursorPos.y + ( mBoxRect.getHeight() - bar.getHeight() ) * yPos ),
					 bar } );
	primitives.drawRoundedRectangle( barRect, 0, Vector2f::One,
									 (int)eefloor( bar.getWidth() * 0.5f ) );
}

void AutoCompletePlugin::drawBeforeLineText( UICodeEditor* editor, const Int64& index, Vector2f,
											 const Float&, const Float& lineHeight ) {
	auto sessionIt = mSnippetSessions.find( &editor->getDocument() );
	if ( sessionIt == mSnippetSessions.end() || sessionIt->second.editor != editor )
		return;
	const auto& session = sessionIt->second;
	if ( session.currentGroup >= session.groups.size() )
		return;
	const auto& group = session.groups[session.currentGroup];
	Primitives primitives;
	const auto& style = editor->getColorScheme().getEditorSyntaxStyle( "matching_selection"_sst );
	primitives.setColor( Color( style.color, 55 ) );
	const Float minimumWidth = PixelDensity::dpToPx( 2.f );
	for ( const auto& occurrence : group.occurrences ) {
		const auto normalized = occurrence.range.normalized();
		if ( !normalized.containsLine( index ) )
			continue;
		auto rectangles = editor->getTextRangeRectangles(
			normalized, editor->getScreenScroll(), DocumentLineRange{ index, index }, lineHeight );
		for ( auto& rectangle : rectangles ) {
			if ( rectangle.getWidth() < minimumWidth )
				rectangle.Right = rectangle.Left + minimumWidth;
			primitives.drawRectangle( rectangle );
		}
	}
}

Rectf AutoCompletePlugin::findBestDocumentationPlacement(
	UICodeEditor* editor, const LSPMarkupContent& suggestion, const std::string& detail,
	const Rectf& anchorBox, const Rectf& rowRect, const Vector2f& cursorScreenPos, bool,
	Float lineHeight ) {
	PopupPlacementConfig config;
	config.areaRect = editor->getScreenRect();
	config.targetRect = anchorBox;
	config.alignRect = rowRect;
	// Small avoid-rect: just the cursor cell + a few character-widths to the right,
	// so the documentation can still sit to the right of the cursor text without being moved.
	Float cursorAvoidX = cursorScreenPos.x + editor->getGlyphWidth() * 4;
	config.avoidRect = Rectf( Vector2f( cursorScreenPos.x, cursorScreenPos.y ),
							  Sizef( cursorAvoidX - cursorScreenPos.x + 1, lineHeight ) );
	// Enable cursor-aware placement: when popup would cover the cursor area, try above target.
	config.cursorScreenPos = cursorScreenPos;
	config.cursorLineHeight = lineHeight;
	config.userMaxWidth = editor->convertLength(
		StyleSheetLength( mMaxSuggestionDocumentationWidth ), editor->getPixelsSize().getWidth() );
	config.minHorizontalSpace = PixelDensity::dpToPx( 200.f );
	config.margin = PixelDensity::dpToPx( 2.f );
	config.minScoreHeight = mRowHeight * 2;
	config.maxScoreHeight = mRowHeight * 10;
	auto measureContent = [&]( Float availableMaxWidth ) -> Sizef {
		Float textWrapWidth =
			std::max( 0.f, availableMaxWidth - mBoxPadding.Left - mBoxPadding.Right );
		mSuggestionDoc.setMaxWrapWidth( textWrapWidth );
		bool changed = mSuggestionDoc.setString( suggestion.value );
		if ( changed ) {
			bool forceHTML = String::startsWith( detail, "Emmet" );
			if ( suggestion.kind == LSPMarkupKind::MarkDown || forceHTML ) {
				const auto& syntaxDef =
					forceHTML ? SyntaxDefinitionManager::instance()->getByLSPName( "html" )
							  : SyntaxDefinitionManager::instance()->getByLSPName( "markdown" );
				SyntaxTokenizer::tokenizeText( syntaxDef, editor->getColorScheme(), &mSuggestionDoc,
											   0, 0xFFFFFFFF, true, "\n\t " );
			}
		}
		return { mSuggestionDoc.getTextWidth() + mBoxPadding.Left + mBoxPadding.Right,
				 mSuggestionDoc.getTextHeight() + mBoxPadding.Top + mBoxPadding.Bottom };
	};
	return UIPlacementUtils::findBestPopupPlacement( config, measureContent ).rect.round();
}

bool AutoCompletePlugin::onMouseDown( UICodeEditor* editor, const Vector2i& position,
									  const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK ) {
		if ( !mSuggestions.empty() && mSuggestionsEditor == editor ) {
			Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
			if ( mBoxRect.contains( localPos ) ) {
				localPos -= { mBoxRect.Left, mBoxRect.Top };
				mSuggestionIndex = mSuggestionsStartIndex + localPos.y / mRowHeight;
				editor->invalidateDraw();
				return true;
			}
		}
		cancelSnippetSession( &editor->getDocument(), true );
	}
	return false;
}

bool AutoCompletePlugin::onMouseUp( UICodeEditor* editor, const Vector2i& position,
									const Uint32& flags ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( mBoxRect.contains( localPos ) ) {
		if ( flags & EE_BUTTON_WUMASK ) {
			mSuggestionsStartIndex = eemax( 0, mSuggestionsStartIndex - mSuggestionsMaxVisible );
			editor->invalidateDraw();
			return true;
		} else if ( flags & EE_BUTTON_WDMASK ) {
			mSuggestionsStartIndex =
				eemax( 0, eemin( (int)mSuggestions.size() - mSuggestionsMaxVisible,
								 mSuggestionsStartIndex + mSuggestionsMaxVisible ) );
			editor->invalidateDraw();
			return true;
		}
	}
	return false;
}

bool AutoCompletePlugin::onMouseDoubleClick( UICodeEditor* editor, const Vector2i& position,
											 const Uint32& flags ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor ||
		 !( flags & EE_BUTTON_LMASK ) )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( mBoxRect.contains( localPos ) ) {
		pickSuggestion( editor );
		return true;
	}
	return false;
}

bool AutoCompletePlugin::onMouseMove( UICodeEditor* editor, const Vector2i& position,
									  const Uint32& ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );

	if ( localPos.x <= editor->getGutterWidth() )
		return false;

	if ( mBoxRect.contains( localPos ) ) {
		editor->getUISceneNode()->setCursor( Cursor::Hand );
		return true;
	} else {
		editor->getUISceneNode()->setCursor( !editor->isLocked() ? Cursor::IBeam : Cursor::Arrow );
	}
	return false;
}

const Rectf& AutoCompletePlugin::getBoxPadding() const {
	return mBoxPadding;
}

void AutoCompletePlugin::setBoxPadding( const Rectf& boxPadding ) {
	mBoxPadding = boxPadding;
}

const Int32& AutoCompletePlugin::getSuggestionsMaxVisible() const {
	return mSuggestionsMaxVisible;
}

void AutoCompletePlugin::setSuggestionsMaxVisible( const Uint32& suggestionsMaxVisible ) {
	mSuggestionsMaxVisible = suggestionsMaxVisible;
}

const Time& AutoCompletePlugin::getUpdateFreq() const {
	return mUpdateFreq;
}

void AutoCompletePlugin::setUpdateFreq( const Time& updateFreq ) {
	mUpdateFreq = updateFreq;
}

const std::string& AutoCompletePlugin::getSymbolPattern() const {
	return mSymbolPattern;
}

void AutoCompletePlugin::setSymbolPattern( const std::string& symbolPattern ) {
	mSymbolPattern = symbolPattern;
}

bool AutoCompletePlugin::isDirty() const {
	return mDirty;
}

void AutoCompletePlugin::setDirty( bool dirty ) {
	mDirty = dirty;
}

void AutoCompletePlugin::resetSuggestions( UICodeEditor* editor ) {
	Lock l( mSuggestionsMutex );
	mSnippetChoiceSuggestions = false;
	mSuggestionIndex = 0;
	mSuggestionsStartIndex = 0;
	{
		Lock l2( mSuggestionsEditorMutex );
		mSuggestionsEditor = nullptr;
	}
	mSuggestions.clear();
	if ( editor && editor->hasFocus() ) {
		auto mousePos( editor->getUISceneNode()->getUIEventDispatcher()->getMousePosf() );
		if ( editor->getScreenRect().contains( mousePos ) )
			editor->updateMouseCursor( mousePos );
	}
}

void AutoCompletePlugin::resetSignatureHelp() {
	mSignatureHelpVisible = false;
	mSignatureHelp.signatures.clear();
	mSignatureHelp.activeSignature = 0;
	mSignatureHelp.activeParameter = 0;
	Lock l( mSignatureHelpEditorMutex );
	mSignatureHelpEditor = nullptr;
}

AutoCompletePlugin::SymbolsList AutoCompletePlugin::getDocumentSymbols( TextDocument* doc ) {
	static constexpr auto MAX_LINE_COUNT = EE_1KB * 10;
	AutoCompletePlugin::SymbolsList symbols;
	std::shared_ptr<TextDocument> docRef =
		getPluginContext()->getSplitter()->getTextDocumentRef( doc ); // acquire a doc
	if ( docRef == nullptr )
		return symbols;
	LuaPattern pattern( mSymbolPattern );
	if ( doc->linesCount() == 0 || doc->isHuge() || mShuttingDown )
		return symbols;
	std::string current( getPartialSymbol( doc ) );
	TextPosition end = doc->getSelection().end();
	auto lineCount = doc->linesCount();
	std::string string;
	for ( Int64 i = 0; i < static_cast<Int64>( lineCount ); i++ ) {
		auto len = doc->getLineLength( i );
		if ( len == 0 || len > MAX_LINE_COUNT ) {
			if ( len == 0 ) // Line count must have changed
				lineCount = doc->linesCount();
			continue;
		}
		doc->getLineTextToBufferUtf8( i, string );
		for ( auto& match : pattern.gmatch( string ) ) {
			std::string matchStr( match[0] );
			// Ignore the symbol if is actually the current symbol being written
			if ( matchStr.size() < 3 || ( end.line() == i && current == matchStr ) )
				continue;
			if ( std::none_of( symbols.begin(), symbols.end(),
							   [matchStr]( const Suggestion& suggestion ) {
								   return suggestion.text == matchStr;
							   } ) )
				symbols.push_back( std::move( matchStr ) );
		}
		if ( mShuttingDown || mDocs.find( doc ) == mDocs.end() )
			break;
	}
	return symbols;
}

void AutoCompletePlugin::runUpdateSuggestions( const std::string& symbol,
											   const SymbolsList& symbols, UICodeEditor* editor,
											   bool fromDocCache ) {
	{
		{
			Lock l( mSuggestionsEditorMutex );
			mSuggestionsEditor = editor;
		}
		if ( tryRequestCapabilities( editor ) )
			requestCodeCompletion( editor );
		if ( symbol.empty() || symbols.empty() )
			return;

		Lock l( fromDocCache ? mDocMutex : mLangSymbolsMutex );
		Lock l2( mSuggestionsMutex );
		mSuggestions = fuzzyMatchSymbols( { &symbols }, symbol, mSuggestionsMaxVisible );
	}
	editor->runOnMainThread( [editor] { editor->invalidateDraw(); } );
}

void AutoCompletePlugin::updateSuggestions( const std::string& symbol, UICodeEditor* editor ) {
	TextDocument& doc = editor->getDocument();
	bool usesOwnSymbols = false;

	{
		Lock l( mDocUsesOwnSymbolsMutex );
		usesOwnSymbols = mDocUsesOwnSymbols[&doc];
	}

	if ( usesOwnSymbols ) {
		Lock l( mDocMutex );
		auto docCache = mDocCache.find( &doc );
		if ( docCache == mDocCache.end() || mShuttingDown )
			return;
		const auto& symbols = docCache->second.symbols;
		{
			mThreadPool->run( [this, symbol, &symbols, editor] {
				runUpdateSuggestions( symbol, symbols, editor, true );
			} );
		}
	}

	const std::string& lang = doc.getSyntaxDefinition().getLanguageName();
	Lock l( mLangSymbolsMutex );
	auto langSuggestions = mLangCache.find( lang );
	if ( langSuggestions == mLangCache.end() )
		return;
	const auto& symbols = langSuggestions->second;
	{
		mThreadPool->run( [this, symbol, &symbols, editor] {
			runUpdateSuggestions( symbol, symbols, editor, false );
		} );
	}
}

bool AutoCompletePlugin::onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
											  const Vector2i& /*position*/,
											  const Uint32& /*flags*/ ) {
	menu->addSeparator();

	bool usesOwnSymbols = false;

	{
		Lock l( mDocUsesOwnSymbolsMutex );
		usesOwnSymbols = mDocUsesOwnSymbols[&editor->getDocument()];
	}

	auto* subMenu = UIPopUpMenu::New();
	subMenu->addClass( "autocomplete_plugin_menu" );
	subMenu
		->addCheckBox(
			i18n( "autocomplete_from_doc_symbols",
				  "Limit autocomplete to symbols in this document" ),
			usesOwnSymbols,
			KeyBindings::keybindFormat( mKeyBindings["autocomplete-from-current-doc-symbols"] ) )
		->setTooltipText(
			i18n( "autocomplete_from_doc_symbols_tooltip",
				  "Instead of using the complete current language dictionary symbols\nit will use "
				  "only the dictionary symbols from the current document." ) )
		->setId( "autocomplete-from-current-doc-symbols" );

	menu->addSubMenu( i18n( "autocomplete", "Auto-Complete" ),
					  mManager->getUISceneNode()
						  ->findIcon( "symbol-string" )
						  ->createDrawable( PixelDensity::dpToPxI( 12 ) ),
					  subMenu );

	return false;
}

} // namespace ecode
