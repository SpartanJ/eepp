#include "autocompleteplugin.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>

#include <algorithm>
#include <nlohmann/json.hpp>
using namespace EE::Graphics;
using namespace EE::System;
using json = nlohmann::json;
using namespace std::literals;

namespace ecode {

static constexpr auto SNIPPET_PTRN1 = "%$%{%d+%}"sv;
static constexpr auto SNIPPET_PTRN2 = "%$%{%d+%:([%w,.%s%+%-]+)}"sv;
static constexpr auto SNIPPET_PTRN3 = "%$%d+"sv;

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
	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [this, editor]( const Event* ) {
			mDirty = true;
			mDocs.insert( editor->getDocumentRef().get() );
			mEditorDocs[editor] = editor->getDocumentRef().get();
			tryRequestCapabilities( editor );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentClosed, [this]( const Event* event ) {
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

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [this, editor]( const Event* ) {
			TextDocument* oldDoc = mEditorDocs[editor];
			TextDocument* newDoc = editor->getDocumentRef().get();

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

	listeners.push_back( editor->addEventListener( Event::OnCursorPosChange, [this, editor](
																				 const Event* ) {
		if ( !mReplacing )
			resetSuggestions( editor );

		if ( mSignatureHelpVisible && mSignatureHelpPosition.isValid() &&
			 !editor->getDocument().getSelection().hasSelection() &&
			 mSignatureHelpPosition.line() != editor->getDocument().getSelection().end().line() ) {
			resetSignatureHelp();
		}
	} ) );

	listeners.push_back( editor->addEventListener(
		Event::OnFocusLoss, [this]( const Event* ) { resetSignatureHelp(); } ) );

	listeners.push_back( editor->addEventListener(
		Event::OnDocumentUndoRedo, [this]( const Event* ) { resetSignatureHelp(); } ) );

	listeners.push_back( editor->addEventListener(
		Event::OnDocumentSyntaxDefinitionChange, [this]( const Event* ev ) {
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
		mDocs.erase( doc );
		mDocCache.erase( doc );
	}

	{
		Lock l( mDocUsesOwnSymbolsMutex );
		mDocUsesOwnSymbols.erase( doc );
	}

	mDirty = true;
}

bool AutoCompletePlugin::onKeyDown( UICodeEditor* editor, const KeyEvent& event ) {
	KeyBindings::Shortcut eventShortcut( event.getKeyCode(), event.getSanitizedMod() );
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
		} else if ( event.getKeyCode() == EE::Window::KEY_BACKSPACE ||
					event.getKeyCode() == EE::Window::KEY_DELETE ) {
			auto lang = editor->getDocumentRef()->getSyntaxDefinition().getLSPName();
			auto cap = mCapabilities.find( lang );
			if ( cap != mCapabilities.end() ) {
				auto curChar = event.getKeyCode() == EE::Window::KEY_BACKSPACE
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
	} else if ( mShortcuts["autocomplete-update-suggestions"] == eventShortcut ) {
		std::string partialSymbol( getPartialSymbol( &editor->getDocument() ) );
		updateSuggestions( partialSymbol, editor );
		return true;
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

void AutoCompletePlugin::pickSuggestion( UICodeEditor* editor ) {
	mReplacing = true;
	std::string symbol( getPartialSymbol( editor->getDocumentRef().get() ) );
	const auto& suggestion = mSuggestions[mSuggestionIndex];
	auto doc = editor->getDocumentRef();
	auto prevSels = doc->getSelections();

	if ( doc->getSelections().size() == 1 && suggestion.range.isValid() &&
		 doc->isValidRange( suggestion.range ) ) {
		doc->setSelection( suggestion.range );
		doc->textInput( !suggestion.insertText.empty() ? suggestion.insertText : suggestion.text );
	} else {
		if ( !symbol.empty() )
			doc->execute( "delete-to-previous-word" );
		doc->textInput( !suggestion.insertText.empty() ? suggestion.insertText : suggestion.text );
	}

	tryStartSnippetNav( suggestion, editor, prevSels );

	mReplacing = false;
	resetSuggestions( editor );
}

void AutoCompletePlugin::tryStartSnippetNav( const Suggestion& suggestion, UICodeEditor* editor,
											 const TextRanges& prevSels ) {
	if ( !hasCompleteSteps( suggestion ) )
		return;

	auto doc = editor->getDocumentRef();
	auto selections = doc->getSelections();
	TextRanges newSelections;
	newSelections.reserve( selections.size() );
	size_t i = 0;
	for ( const auto& sel : selections ) {
		newSelections.emplace_back( prevSels[i].start(), sel.end() );
		i++;
	}
	std::vector<TextRanges> ranges;

	for ( const auto& sel : newSelections ) {
		TextRanges steps;

		auto res = doc->findAll( SNIPPET_PTRN1, true, false,
								 TextDocument::FindReplaceType::LuaPattern, sel );

		auto res2 = doc->findAll( SNIPPET_PTRN2, true, false,
								  TextDocument::FindReplaceType::LuaPattern, sel );

		auto res3 = doc->findAll( SNIPPET_PTRN3, true, false,
								  TextDocument::FindReplaceType::LuaPattern, sel );

		res.reserve( res.size() + res2.size() + res3.size() );

		for ( auto& r : res2 )
			res.emplace_back( std::move( r ) );

		for ( auto& r : res3 )
			res.emplace_back( std::move( r ) );

		if ( res.empty() )
			continue;

		std::sort(
			res.begin(), res.end(),
			[]( const TextDocument::SearchResult& left, const TextDocument::SearchResult& right ) {
				return left.result > right.result;
			} );

		for ( TextDocument::SearchResult& sr : res ) {
			if ( !sr.isValid() )
				continue;

			doc->setSelection( sr.result );

			if ( !sr.captures.empty() ) {
				auto text = doc->getText( sr.captures[0] );
				auto pos = doc->replaceSelection( text );

				for ( auto& step : steps ) {
					if ( step.start().line() == sr.result.start().line() ) {
						Int64 offset = sr.result.length() - text.size();
						step.setStart( doc->positionOffset( step.start(), -offset, false ) );
						step.setEnd( doc->positionOffset( step.end(), -offset, false ) );
					}
				}

				steps.emplace_back( pos, doc->positionOffset( pos, -text.size() ) );
			} else {
				auto pos = doc->replaceSelection( "" );

				for ( auto& step : steps ) {
					if ( step.start().line() == sr.result.start().line() ) {
						Int64 offset = sr.result.length();
						step.setStart( doc->positionOffset( step.start(), -offset, false ) );
						step.setEnd( doc->positionOffset( step.end(), -offset, false ) );
					}
				}

				steps.emplace_back( pos, pos );
			}
		}

		ranges.emplace_back( steps );
	}

	TextRanges initialRanges;

	for ( const auto& textRanges : ranges ) {
		TextRange sel;
		for ( const auto& range : textRanges ) {
			// skip ranges that have default values
			if ( range.hasSelection() )
				continue;
			sel = range;
		}
		if ( sel.isValid() )
			initialRanges.push_back( sel );
	}
	doc->setSelection( initialRanges.empty() ? prevSels : initialRanges );
}

bool AutoCompletePlugin::hasCompleteSteps( const Suggestion& suggestion ) {
	if ( suggestion.kind != LSPCompletionItemKind::Snippet )
		return false;
	if ( LuaPattern::hasMatches( suggestion.insertText, SNIPPET_PTRN1 ) ||
		 LuaPattern::hasMatches( suggestion.insertText, SNIPPET_PTRN2 ) ||
		 LuaPattern::hasMatches( suggestion.insertText, SNIPPET_PTRN3 ) ) {
		return true;
	}
	return false;
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
			suggestions.push_back(
				{ item.kind, std::move( item.label.empty() ? item.insertText : item.label ),
				  std::move( item.detail ), std::move( item.sortText ), item.textEdit.range,
				  std::move( item.textEdit.text ), std::move( item.documentation ) } );
		} else if ( !item.insertText.empty() ) {
			suggestions.push_back(
				{ item.kind, std::move( item.label.empty() ? item.insertText : item.label ),
				  std::move( item.detail ), std::move( item.sortText ), item.textEdit.range,
				  std::string{ item.insertText }, std::move( item.documentation ) } );
		} else {
			suggestions.push_back( { item.kind,
									 std::move( item.filterText ),
									 std::move( item.detail ),
									 std::move( item.sortText ),
									 {},
									 "",
									 std::move( item.documentation ) } );
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
	// This guarantees that we always display a single line signature help
	SignatureHelp signatures;
	signatures.activeSignature = signatureHelp.activeSignature;
	signatures.activeParameter = signatureHelp.activeParameter;
	signatures.signatures.reserve( signatureHelp.signatures.size() );

	TextDocument doc;
	for ( const auto& sig : signatureHelp.signatures ) {
		String initialLabel( sig.label );
		SignatureInformation nsig;
		nsig.documentation = sig.documentation;

		doc.reset();
		doc.textInput( initialLabel );
		std::vector<String> parameters;
		parameters.reserve( sig.parameters.size() );

		for ( size_t i = 0; i < sig.parameters.size(); i++ ) {
			auto start = String::utf8ToCodepointPosition( sig.label, sig.parameters[i].start );
			auto end = String::utf8ToCodepointPosition( sig.label, sig.parameters[i].end );
			auto sel = TextRange::convertToLineColumn( initialLabel.view(), start, end );

			if ( i == 0 )
				doc.setSelection( i, sel );
			else
				doc.addSelection( sel );

			parameters.emplace_back( doc.getSelectedText( i ) );
		}

		auto selections( doc.getSelections() );
		nsig.parameters.reserve( selections.size() );

		if ( 0 != doc.replaceAll( "\n", "" ) ) {
			while ( 0 != doc.replaceAll( "  ", " " ) )
				;

			nsig.label = doc.getLineTextWithoutNewLine( 0 );

			for ( const auto& param : parameters ) {
				auto res = doc.find( param );
				if ( res.isValid() )
					nsig.parameters.emplace_back( res.result );
			}
		} else {
			nsig.label = std::move( initialLabel );

			if ( !sig.parameters.empty() ) {
				for ( auto& sel : selections )
					nsig.parameters.emplace_back( sel );
			}
		}

		signatures.signatures.emplace_back( nsig );
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

void AutoCompletePlugin::update( UICodeEditor* ) {
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
											const Float& /*lineHeight*/, bool drawUp ) {

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
	Float vdiff = drawUp ? -mRowHeight : mRowHeight;
	auto offset = editor->getTextPositionOffset( mSignatureHelpPosition );
	Vector2f pos( startScroll.x + offset.x, startScroll.y + offset.y + vdiff );
	primitives.setColor( Color( selectedStyle.background ).blendAlpha( editor->getAlpha() ) );
	String str;
	if ( mSignatureHelp.signatures.size() > 1 ) {
		str = String::format( "%s (%d of %zu)", curSig.label.toUtf8(),
							  mSignatureHelpSelected == -1 ? 1 : mSignatureHelpSelected + 1,
							  mSignatureHelp.signatures.size() );
	} else {
		str = curSig.label;
	}

	Rectf boxRect( pos, Sizef( editor->getTextWidth( str ) + mBoxPadding.Left + mBoxPadding.Right,
							   mRowHeight ) );
	if ( boxRect.getPosition().x + boxRect.getSize().getWidth() >
		 editor->getScreenPos().x + editor->getPixelsSize().getWidth() ) {
		boxRect.setPosition(
			{ eefloor( editor->getScreenPos().x + editor->getPixelsSize().getWidth() -
					   boxRect.getSize().getWidth() ),
			  boxRect.getPosition().y } );
		if ( boxRect.getPosition().x < editor->getScreenPos().x )
			boxRect.setPosition( { eefloor( editor->getScreenPos().x ), boxRect.getPosition().y } );
	}

	bool hasParams = !curSig.parameters.empty();
	TextRange curParam =
		hasParams ? curSig.parameters[mSignatureHelp.activeParameter % curSig.parameters.size()]
				  : TextRange{};
	Rectf curParamRect;
	if ( hasParams ) {
		curParamRect = Rectf(
			{ { boxRect.getPosition().x + mBoxPadding.Left +
					curParam.start().column() * editor->getGlyphWidth(),
				boxRect.getPosition().y },
			  { ( curParam.end().column() - curParam.start().column() ) * editor->getGlyphWidth(),
				mRowHeight } } );

		if ( !editor->getScreenRect().contains(
				 Rectf{ { curParamRect.getPosition().x +
							  ( curParam.end().column() - curParam.start().column() ) *
								  editor->getGlyphWidth(),
						  curParamRect.getPosition().y },
						curParamRect.getSize() } ) ) {
			auto offset = editor->getTextPositionOffset( mSignatureHelpPosition );
			pos = { static_cast<Float>( startScroll.x -
										curParam.start().column() * editor->getGlyphWidth() +
										offset.x ),
					static_cast<Float>( startScroll.y + offset.y + vdiff ) };

			boxRect.setPosition( pos );

			curParamRect.setPosition( { boxRect.getPosition().x + mBoxPadding.Left +
											curParam.start().column() * editor->getGlyphWidth(),
										boxRect.getPosition().y } );
		}
	}

	primitives.drawRoundedRectangle( boxRect, 0.f, Vector2f::One, 6 );

	if ( hasParams && curParam.end() != curParam.start() &&
		 curParam.end().column() < (int)str.size() ) {
		primitives.setColor( matchingSelection.color );
		primitives.drawRoundedRectangle( curParamRect, 0.f, Vector2f::One, 6 );
	}

	Text text( "", editor->getFont(), editor->getFontSize() );
	text.setFillColor( normalStyle.color );
	text.setStyle( normalStyle.style );
	text.setString( str );
	SyntaxTokenizer::tokenizeText( doc.getSyntaxDefinition(), editor->getColorScheme(), &text );
	text.draw( boxRect.getPosition().x + mBoxPadding.Left,
			   boxRect.getPosition().y + mBoxPadding.Top );
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

		Drawable* icon = editor->getUISceneNode()->findIconDrawable(
			LSPCompletionItemHelper::toIconString( suggestion.kind ), PixelDensity::dpToPxI( 12 ) );

		if ( icon ) {
			Color iconColor( icon->getColor() );
			icon->setColor( mSuggestionIndex == (int)i ? selectedStyle.color : normalStyle.color );
			Vector2f padding(
				eefloor( ( iconSpace.getWidth() - icon->getSize().getWidth() ) * 0.5f ),
				eefloor( ( iconSpace.getHeight() - icon->getSize().getHeight() ) * 0.5f ) );
			icon->draw( { cursorPos.x + padding.x, cursorPos.y + mRowHeight * count + padding.y } );
			icon->setColor( iconColor );
		}

		if ( mSuggestionIndex == (int)i && !suggestion.documentation.value.empty() ) {
			mSuggestionDoc.setFont( editor->getFont() );
			mSuggestionDoc.setFontSize( editor->getFontSize() );
			mSuggestionDoc.setFillColor( normalStyle.color );
			mSuggestionDoc.setStyle( normalStyle.style );
			bool changed = mSuggestionDoc.setString( suggestion.documentation.value );

			Vector2f boxPos = { cursorPos.x + mBoxRect.getWidth(),
								cursorPos.y + mRowHeight * count };
			Sizef boxSize = { mSuggestionDoc.getTextWidth() + mBoxPadding.Left + mBoxPadding.Right,
							  mSuggestionDoc.getTextHeight() + mBoxPadding.Top +
								  mBoxPadding.Bottom };
			primitives.setColor(
				Color( selectedStyle.background ).blendAlpha( editor->getAlpha() ) );
			primitives.drawRoundedRectangle( { boxPos, boxSize }, 0.f, Vector2f::One, 6 );

			if ( changed ) {
				bool forceHTML = String::startsWith( suggestion.detail, "Emmet" );
				if ( suggestion.documentation.kind == LSPMarkupKind::MarkDown || forceHTML ) {
					const auto& syntaxDef =
						forceHTML ? SyntaxDefinitionManager::instance()->getByLSPName( "html" )
								  : SyntaxDefinitionManager::instance()->getByLSPName( "markdown" );
					SyntaxTokenizer::tokenizeText( syntaxDef, editor->getColorScheme(),
												   &mSuggestionDoc, 0, 0xFFFFFFFF, true, "\n\t " );
				}
			}

			mSuggestionDoc.draw( boxPos.x + mBoxPadding.Left, boxPos.y + mBoxPadding.Top );
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

bool AutoCompletePlugin::onMouseDown( UICodeEditor* editor, const Vector2i& position,
									  const Uint32& flags ) {
	if ( mSuggestions.empty() || !mSuggestionsEditor || mSuggestionsEditor != editor ||
		 !( flags & EE_BUTTON_LMASK ) )
		return false;
	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( mBoxRect.contains( localPos ) ) {
		localPos -= { mBoxRect.Left, mBoxRect.Top };
		mSuggestionIndex = mSuggestionsStartIndex + localPos.y / mRowHeight;
		editor->invalidateDraw();
		return true;
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
	static constexpr auto MAX_LINE_LENGTH = EE_1KB * 10;
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
		if ( doc->getLineLength( i ) > MAX_LINE_LENGTH )
			continue;
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
						  ->getSize( PixelDensity::dpToPxI( 12 ) ),
					  subMenu );

	return false;
}

} // namespace ecode
