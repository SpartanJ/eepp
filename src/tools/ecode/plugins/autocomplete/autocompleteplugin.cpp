#include "autocompleteplugin.hpp"
#include "../../settingspage.hpp"
#include "../../universallocator.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/math/math.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/uuid.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>

#include <algorithm>
#include <array>
#include <ctime>
#include <nlohmann/json.hpp>
#include <string_view>
using namespace EE::Graphics;
using namespace EE::System;
using json = nlohmann::json;
using namespace std::literals;

namespace ecode {

void AutoCompletePlugin::registerSettings( SettingsPage& page ) {
	page.addGroup( i18n( "general", "General" ) );
	page.addBool(
		"suggestions-syntax-highlight", "/config/suggestions_syntax_highlight",
		i18n( "autocomplete_suggestions_syntax_highlight", "Syntax Highlight Suggestions" ),
		i18n( "autocomplete_suggestions_syntax_highlight_desc",
			  "Apply syntax highlighting to completion suggestions." ),
		true );
	page.addInteger( "max-label-characters", "/config/max_label_characters",
					 i18n( "autocomplete_max_label_characters", "Maximum Label Characters" ),
					 i18n( "autocomplete_max_label_characters_desc",
						   "Maximum number of characters shown in a suggestion label." ),
					 1, 10000, 100 );
	auto cssLength = []( const std::string& text ) { return StyleSheetLength::isLength( text ); };
	page.addText( "max-suggestion-documentation-width",
				  "/config/max_suggestion_documentation_width",
				  i18n( "autocomplete_max_suggestion_documentation_width",
						"Maximum Suggestion Documentation Width" ),
				  i18n( "autocomplete_max_suggestion_documentation_width_desc",
						"Maximum documentation popup width as a CSS length." ),
				  "100%", cssLength );
	page.addText( "max-signature-helper-width", "/config/max_signature_helper_width",
				  i18n( "autocomplete_max_signature_helper_width", "Maximum Signature Help Width" ),
				  i18n( "autocomplete_max_signature_helper_width_desc",
						"Maximum signature help popup width as a CSS length." ),
				  "90%", cssLength );
	page.addBool( "signature-help-multi-line", "/config/signature_help_multi_line",
				  i18n( "autocomplete_signature_help_multi_line", "Multiline Signature Help" ),
				  i18n( "autocomplete_signature_help_multi_line_desc",
						"Allow signature help to use multiple lines." ),
				  true );
	page.addBool( "suggestion-documentation", "/config/suggestion_documentation",
				  i18n( "autocomplete_suggestion_documentation", "Suggestion Documentation" ),
				  i18n( "autocomplete_suggestion_documentation_desc",
						"Show documentation alongside completion suggestions." ),
				  true );
	page.addBool(
		"signature-help-documentation", "/config/signature_help_documentation",
		i18n( "autocomplete_signature_help_documentation", "Signature Help Documentation" ),
		i18n( "autocomplete_signature_help_documentation_desc",
			  "Show documentation alongside signature help." ),
		true );
	page.addBool( "load-vscode-snippets", "/config/load_vscode_snippets",
				  i18n( "autocomplete_load_vscode_snippets", "Load VS Code Snippets" ),
				  i18n( "autocomplete_load_vscode_snippets_desc",
						"Load compatible snippets installed for VS Code." ),
				  true );
}

class SnippetLocatorModel : public Model {
  public:
	struct Row {
		std::string name;
		std::string prefixes;
		std::string detail;
		std::string body;
	};

	explicit SnippetLocatorModel( std::vector<UserSnippetMatch> matches ) {
		mRows.reserve( matches.size() );
		for ( auto& match : matches ) {
			std::string prefixes;
			for ( const auto& prefix : match.snippet.prefixes ) {
				if ( !prefixes.empty() )
					prefixes += ", ";
				prefixes += prefix;
			}
			const char* source = match.snippet.source == UserSnippetSource::EcodeProject ? ".ecode"
								 : match.snippet.source == UserSnippetSource::VSCodeProject
									 ? ".vscode"
									 : "user";
			std::string detail( std::move( match.snippet.description ) );
			if ( !detail.empty() )
				detail += " — ";
			detail += source;
			mRows.push_back( { std::move( match.snippet.name ), std::move( prefixes ),
							   std::move( detail ), std::move( match.snippet.body ) } );
		}
	}

	size_t rowCount( const ModelIndex& ) const override { return mRows.size(); }

	size_t columnCount( const ModelIndex& ) const override { return 3; }

	std::string columnName( const size_t& column ) const override {
		static constexpr std::array<std::string_view, 3> names{ "Name", "Prefixes", "Description" };
		return std::string{ names[column] };
	}

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		if ( !index.isValid() || index.row() >= static_cast<Int64>( mRows.size() ) )
			return {};
		const auto& row = mRows[index.row()];
		if ( role == ModelRole::Custom )
			return Variant{ row.body };
		if ( role != ModelRole::Display )
			return {};
		switch ( index.column() ) {
			case 0:
				return Variant{ row.name };
			case 1:
				return Variant{ row.prefixes };
			case 2:
				return Variant{ row.detail };
		}
		return {};
	}

  private:
	std::vector<Row> mRows;
};

static bool pathStartsWith( std::string_view path, std::string_view prefix ) {
	return !prefix.empty() && path.size() >= prefix.size() &&
		   path.compare( 0, prefix.size(), prefix ) == 0;
}

static bool pathEndsWith( std::string_view path, std::string_view suffix ) {
	return path.size() >= suffix.size() &&
		   path.compare( path.size() - suffix.size(), suffix.size(), suffix ) == 0;
}

static bool getSnippetPathSource( std::string_view path, std::string_view userPath,
								  std::string_view vscodePath, std::string_view ecodePath,
								  UserSnippetSource& source, bool& languageFiles ) {
	if ( pathStartsWith( path, userPath ) &&
		 ( pathEndsWith( path, ".json" ) || pathEndsWith( path, ".code-snippets" ) ) ) {
		source = UserSnippetSource::User;
		languageFiles = true;
		return true;
	}
	if ( pathEndsWith( path, ".code-snippets" ) ) {
		if ( pathStartsWith( path, vscodePath ) ) {
			source = UserSnippetSource::VSCodeProject;
			languageFiles = false;
			return true;
		}
		if ( pathStartsWith( path, ecodePath ) ) {
			source = UserSnippetSource::EcodeProject;
			languageFiles = false;
			return true;
		}
	}
	return false;
}

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
	for ( const auto& symbols : symbolsVec ) {
		size_t sourceMatches = 0;
		for ( const auto& symbol : *symbols ) {
			const bool serverFilteredSnippet =
				symbol.source == AutoCompletePlugin::Suggestion::Source::LSP &&
				symbol.kind == LSPCompletionItemKind::Snippet;
			const int score =
				serverFilteredSnippet
					? 0
					: String::fuzzyMatchSimple( pattern, symbol.text, false,
												symbol.kind != LSPCompletionItemKind::Text );
			if ( serverFilteredSnippet || score > 0 ) {
				if ( std::find( matches.begin(), matches.end(), symbol ) == matches.end() ) {
					symbol.setScore( score +
									 ( symbol.kind != LSPCompletionItemKind::Text ? score : 0 ) );
					matches.push_back( symbol );
					++sourceMatches;

					if ( sourceMatches >= max )
						break;
				}
			}
		}
	}

	std::sort( matches.begin(), matches.end(),
			   []( const AutoCompletePlugin::Suggestion& left,
				   const AutoCompletePlugin::Suggestion& right ) {
				   if ( left.score != right.score )
					   return left.score > right.score;
				   if ( left.source == AutoCompletePlugin::Suggestion::Source::UserSnippet &&
						right.source == AutoCompletePlugin::Suggestion::Source::UserSnippet &&
						left.sourcePriority != right.sourcePriority )
					   return left.sourcePriority > right.sourcePriority;
				   return left.text < right.text;
			   } );
	if ( matches.size() > max )
		matches.erase( matches.begin() + max, matches.end() );

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
	unregisterSnippetLocatorProvider();
	mManager->unsubscribeMessages( this );
	unsubscribeFileSystemListener();
	while ( mSnippetJobs > 0 )
		Sys::sleep( Milliseconds( 1 ) );
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
			config["signature_help_documentation"] = mSignatureHelpDocumentation;
			updateConfigFile = true;
		}

		if ( config.contains( "load_vscode_snippets" ) )
			mLoadVSCodeSnippets = config.value( "load_vscode_snippets", true );
		else {
			config["load_vscode_snippets"] = mLoadVSCodeSnippets;
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

	mUserSnippetsPath = pluginManager->getConfigPath() + "snippets" + FileSystem::getOSSlash();
	++mSnippetJobs;
	mThreadPool->run( [this, path = mUserSnippetsPath] {
		ScopedOp job( [] {}, [this] { --mSnippetJobs; } );
		Lock lock( mSnippetLoadMutex );
		if ( mShuttingDown )
			return;
		FileSystem::makeDir( path, true );
		loadSnippetDirectory( path, UserSnippetSource::User, true );
	} );
	setSnippetWorkspaceFolder( pluginManager->getWorkspaceFolder() );

	subscribeFileSystemListener();
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

void AutoCompletePlugin::loadSnippetDirectory( const std::string& path, UserSnippetSource source,
											   bool languageFiles ) {
	if ( path.empty() || !FileSystem::isDirectory( path ) )
		return;
	for ( const auto& name : FileSystem::filesGetInPath( path, true, false, true ) ) {
		if ( mShuttingDown )
			return;
		const std::string filePath = path + name;
		if ( !FileSystem::isDirectory( filePath ) )
			loadSnippetFile( filePath, source, languageFiles );
	}
}

void AutoCompletePlugin::loadSnippetFile( const std::string& path, UserSnippetSource source,
										  bool languageFiles ) {
	const std::string extension = FileSystem::fileExtension( path );
	if ( extension != "code-snippets" && ( !languageFiles || extension != "json" ) )
		return;
	std::string contents;
	if ( !FileSystem::fileGet( path, contents ) )
		return;
	std::string defaultScope;
	if ( extension == "json" )
		defaultScope = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) );
	std::vector<std::string> diagnostics;
	if ( !mUserSnippetStore.updateFile( contents, path, source, std::move( defaultScope ),
										&diagnostics ) ) {
		Log::warning( "AutoCompletePlugin: keeping the last valid snippets for invalid file %s",
					  path.c_str() );
	}
	for ( const auto& diagnostic : diagnostics )
		Log::warning( "AutoCompletePlugin: %s", diagnostic.c_str() );
}

void AutoCompletePlugin::setSnippetWorkspaceFolder( std::string workspaceFolder ) {
	if ( !workspaceFolder.empty() )
		FileSystem::dirAddSlashAtEnd( workspaceFolder );
	Uint64 generation;
	{
		Lock lock( mSnippetLoadMutex );
		if ( mSnippetWorkspaceFolder == workspaceFolder )
			return;
		mSnippetWorkspaceFolder = workspaceFolder;
		mVSCodeSnippetsPath = workspaceFolder.empty() || !mLoadVSCodeSnippets
								  ? ""
								  : workspaceFolder + ".vscode" + FileSystem::getOSSlash();
		mEcodeSnippetsPath =
			workspaceFolder.empty() ? "" : workspaceFolder + ".ecode" + FileSystem::getOSSlash();
		generation = ++mSnippetWorkspaceGeneration;
	}
	++mSnippetJobs;
	mThreadPool->run( [this, workspaceFolder = std::move( workspaceFolder ), generation] {
		ScopedOp job( [] {}, [this] { --mSnippetJobs; } );
		Lock lock( mSnippetLoadMutex );
		if ( mShuttingDown || generation != mSnippetWorkspaceGeneration )
			return;
		mUserSnippetStore.removeSource( UserSnippetSource::VSCodeProject );
		mUserSnippetStore.removeSource( UserSnippetSource::EcodeProject );
		if ( workspaceFolder.empty() )
			return;
		if ( mLoadVSCodeSnippets )
			loadSnippetDirectory( workspaceFolder + ".vscode" + FileSystem::getOSSlash(),
								  UserSnippetSource::VSCodeProject, false );
		loadSnippetDirectory( workspaceFolder + ".ecode" + FileSystem::getOSSlash(),
							  UserSnippetSource::EcodeProject, false );
	} );
}

void AutoCompletePlugin::scheduleSnippetFileUpdate( std::string path, UserSnippetSource source,
													bool languageFiles, bool remove ) {
	const Uint64 generation = mSnippetWorkspaceGeneration;
	++mSnippetJobs;
	mThreadPool->run( [this, path = std::move( path ), source, languageFiles, remove, generation] {
		ScopedOp job( [] {}, [this] { --mSnippetJobs; } );
		Lock lock( mSnippetLoadMutex );
		if ( mShuttingDown ||
			 ( source != UserSnippetSource::User && generation != mSnippetWorkspaceGeneration ) )
			return;
		if ( remove )
			mUserSnippetStore.removeFile( path );
		else
			loadSnippetFile( path, source, languageFiles );
	} );
}

void AutoCompletePlugin::onLoadProject( const std::string& projectFolder,
										const std::string& /*projectStatePath*/ ) {
	setSnippetWorkspaceFolder( projectFolder );
}

void AutoCompletePlugin::registerSnippetLocatorProvider() {
	if ( mSnippetLocatorProviderId != 0 || !getPluginContext() ||
		 !getPluginContext()->getUniversalLocator() )
		return;
	auto* locator = getPluginContext()->getUniversalLocator();
	mSnippetLocatorProviderId = locator->registerLocatorProvider(
		{ "sn", i18n( "insert_snippet", "Insert Snippet" ), nullptr,
		  [this, locator]( const Variant&, const ModelEvent* event ) {
			  if ( !event || !event->getModel() || !getPluginContext() )
				  return;
			  auto* editor = getPluginContext()->getSplitter()->getCurEditor();
			  if ( !editor )
				  return;
			  const auto body = event->getModel()->data(
				  event->getModel()->index( event->getModelIndex().row(), 0 ), ModelRole::Custom );
			  if ( !body.isValid() )
				  return;
			  mReplacing = true;
			  insertSnippet( editor, body.toString() );
			  mReplacing = false;
			  resetSuggestions( editor );
			  locator->hideLocateBar();
			  editor->setFocus();
		  },
		  nullptr, false, false,
		  [this]( const String& query, UniversalLocator::LocatorProvider::ModelReadyFn ready ) {
			  if ( !getPluginContext() || !getPluginContext()->getSplitter() ) {
				  ready( std::make_shared<SnippetLocatorModel>( std::vector<UserSnippetMatch>{} ) );
				  return;
			  }
			  auto* editor = getPluginContext()->getSplitter()->getCurEditor();
			  if ( !editor ) {
				  ready( std::make_shared<SnippetLocatorModel>( std::vector<UserSnippetMatch>{} ) );
				  return;
			  }
			  std::string language = editor->getDocument().getSyntaxDefinition().getLSPName();
			  std::string filePath = editor->getDocument().getFilePath();
			  FileSystem::filePathRemoveBasePath( getPluginContext()->getCurrentProject(),
												  filePath );
			  std::string pattern = query.toUtf8();
			  ++mSnippetJobs;
			  mThreadPool->run( [this, language = std::move( language ),
								 filePath = std::move( filePath ), pattern = std::move( pattern ),
								 ready = std::move( ready )] {
				  ScopedOp job( [] {}, [this] { --mSnippetJobs; } );
				  if ( mShuttingDown )
					  return;
				  ready( std::make_shared<SnippetLocatorModel>(
					  mUserSnippetStore.findForLocator( language, pattern, 100, filePath ) ) );
			  } );
		  } } );
}

void AutoCompletePlugin::unregisterSnippetLocatorProvider() {
	if ( mSnippetLocatorProviderId == 0 || !getPluginContext() ||
		 !getPluginContext()->getUniversalLocator() )
		return;
	getPluginContext()->getUniversalLocator()->unregisterLocatorProvider(
		mSnippetLocatorProviderId );
	mSnippetLocatorProviderId = 0;
}

void AutoCompletePlugin::onFileSystemEvent( const FileEvent& ev, const FileInfo& file ) {
	Plugin::onFileSystemEvent( ev, file );
	if ( mShuttingDown || isLoading() )
		return;

	std::string_view path = file.getFilepath();
	UserSnippetSource source;
	bool languageFiles = false;
	bool isSnippetPath;
	const bool updatesSnippet =
		ev.type == FileSystemEventType::Delete || ev.type == FileSystemEventType::Add ||
		ev.type == FileSystemEventType::Modified || ev.type == FileSystemEventType::Moved;
	std::string scheduledPath;
	{
		Lock lock( mSnippetLoadMutex );
		if ( path.empty() ) {
			mSnippetEventPathBuffer.clear();
			mSnippetEventPathBuffer.reserve( ev.directory.size() + ev.filename.size() );
			mSnippetEventPathBuffer.append( ev.directory ).append( ev.filename );
			path = mSnippetEventPathBuffer;
		}
		isSnippetPath = getSnippetPathSource( path, mUserSnippetsPath, mVSCodeSnippetsPath,
											  mEcodeSnippetsPath, source, languageFiles );
		if ( isSnippetPath && updatesSnippet )
			scheduledPath = path;
	}
	if ( isSnippetPath && updatesSnippet ) {
		if ( ev.type == FileSystemEventType::Delete )
			scheduleSnippetFileUpdate( std::move( scheduledPath ), source, languageFiles, true );
		else if ( ev.type == FileSystemEventType::Add || ev.type == FileSystemEventType::Modified ||
				  ev.type == FileSystemEventType::Moved )
			scheduleSnippetFileUpdate( std::move( scheduledPath ), source, languageFiles, false );
	}

	if ( ev.type == FileSystemEventType::Moved && !ev.oldFilename.empty() ) {
		std::string oldPath = ev.oldFilename;
		if ( FileSystem::isRelativePath( oldPath ) ) {
			std::string directory = ev.directory;
			FileSystem::dirAddSlashAtEnd( directory );
			oldPath = directory + oldPath;
		}
		{
			Lock lock( mSnippetLoadMutex );
			isSnippetPath = getSnippetPathSource( oldPath, mUserSnippetsPath, mVSCodeSnippetsPath,
												  mEcodeSnippetsPath, source, languageFiles );
		}
		if ( isSnippetPath )
			scheduleSnippetFileUpdate( oldPath, source, languageFiles, true );
	}
}

FileSystemListenerOptions AutoCompletePlugin::getFileSystemListenerOptions() const {
	auto options = Plugin::getFileSystemListenerOptions();
	const auto eventTypes = fileEventTypeMask( FileSystemEventType::Add ) |
							fileEventTypeMask( FileSystemEventType::Delete ) |
							fileEventTypeMask( FileSystemEventType::Modified ) |
							fileEventTypeMask( FileSystemEventType::Moved );
	for ( const auto* path : { &mUserSnippetsPath, &mVSCodeSnippetsPath, &mEcodeSnippetsPath } ) {
		if ( path->empty() )
			continue;
		FileSystemListenerFilter filter;
		filter.eventTypes = eventTypes;
		filter.path = *path;
		options.filters.emplace_back( std::move( filter ) );
	}
	return options;
}

void AutoCompletePlugin::onRegister( UICodeEditor* editor ) {
	registerSnippetLocatorProvider();
	Lock l( mDocMutex );
	std::vector<Uint32> listeners;
	listeners.push_back( editor->on( Event::OnDocumentLoaded, [this, editor]( const Event* ) {
		mDirty = true;
		{
			Lock l( mDocMutex );
			mDocs.insert( editor->getDocumentRef().get() );
			mEditorDocs[editor] = editor->getDocumentRef().get();
		}
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
			mSnippetChoiceSuggestions && ( mShortcuts.prevSuggestion == eventShortcut ||
										   mShortcuts.nextSuggestion == eventShortcut ||
										   mShortcuts.firstSuggestion == eventShortcut ||
										   mShortcuts.lastSuggestion == eventShortcut ||
										   mShortcuts.prevSuggestionPage == eventShortcut ||
										   mShortcuts.nextSuggestionPage == eventShortcut );
		if ( !choiceNavigationShortcut && isSnippetNavigationCommand( command ) )
			cancelSnippetSession( &editor->getDocument(), true );
	}
	if ( mSignatureHelpVisible ) {
		if ( mShortcuts.closeSignatureHelp == eventShortcut ) {
			resetSignatureHelp();
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts.prevSignatureHelp == eventShortcut ) {
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
		} else if ( mShortcuts.nextSignatureHelp == eventShortcut ) {
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

	if ( !mSnippetChoiceSuggestions && mShortcuts.updateSuggestions == eventShortcut ) {
		std::string partialSymbol( getPartialSymbol( &editor->getDocument() ) );
		updateSuggestions( partialSymbol, editor );
		return true;
	}

	if ( !mSuggestions.empty() ) {
		if ( mShortcuts.nextSuggestion == eventShortcut ) {
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
		} else if ( mShortcuts.prevSuggestion == eventShortcut ) {
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
		} else if ( mShortcuts.closeSuggestion == eventShortcut ) {
			resetSuggestions( editor );
			resetSignatureHelp();
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts.firstSuggestion == eventShortcut ) {
			mSuggestionIndex = 0;
			mSuggestionsStartIndex = 0;
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts.lastSuggestion == eventShortcut ) {
			mSuggestionIndex = mSuggestions.size() - 1;
			mSuggestionsStartIndex = eemax( 0, (int)mSuggestions.size() - mSuggestionsMaxVisible );
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts.prevSuggestionPage == eventShortcut ) {
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
		} else if ( mShortcuts.nextSuggestionPage == eventShortcut ) {
			if ( mSuggestionIndex + mSuggestionsMaxVisible < (int)mSuggestions.size() ) {
				mSuggestionIndex += mSuggestionsMaxVisible - 1;
			} else {
				mSuggestionIndex = mSuggestions.size() - 1;
			}
			mSuggestionsStartIndex =
				eemax<int>( 0, mSuggestionIndex - ( mSuggestionsMaxVisible - 1 ) );
			editor->invalidateDraw();
			return true;
		} else if ( mShortcuts.pickSuggestion == eventShortcut ||
					mShortcuts.pickSuggestionAlt == eventShortcut ||
					mShortcuts.pickSuggestionAlt2 == eventShortcut ) {
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
	const bool hasSnippetInput =
		mUserSnippetStore.size() > 0 && !getUserSnippetInput( editor ).empty();

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
			if ( partialSymbol.size() >= 1 || hasSnippetInput ||
				 std::find( triggerCharacters.begin(), triggerCharacters.end(), event.getChar() ) !=
					 triggerCharacters.end() ) {
				updateSuggestions( partialSymbol, editor );
			} else {
				resetSuggestions( editor );
			}
		}
		return false;
	}

	if ( partialSymbol.size() >= 3 || hasSnippetInput ) {
		updateSuggestions( partialSymbol, editor );
	} else {
		resetSuggestions( editor );
	}
	return false;
}

void AutoCompletePlugin::updateDocCache( std::shared_ptr<TextDocument> doc ) {
	TextDocument* docPtr = doc.get();
	ScopedOp op( [] {},
				 [this, docPtr] {
					 Lock lu( mDocsUpdatingMutex );
					 mDocsUpdating[docPtr] = false;
				 } );

	Clock clock;
	std::unordered_map<TextDocument*, DocCache>::iterator docCache;
	{
		Lock l( mDocMutex );
		docCache = mDocCache.find( docPtr );
		if ( docCache == mDocCache.end() || mShuttingDown )
			return;
	}

	auto changeId = doc->getCurrentChangeId();
	auto symbols = getDocumentSymbols( doc );

	{
		Lock l( mDocMutex );
		docCache = mDocCache.find( docPtr );
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

static std::string formatSnippetTime( const std::tm& time, const char* format ) {
	char buffer[128];
	return std::strftime( buffer, sizeof( buffer ), format, &time ) > 0 ? buffer : "";
}

SnippetParser::VariableMap AutoCompletePlugin::snippetVariables( TextDocument& doc,
																 const TextRange& selection,
																 size_t cursorIndex ) const {
	const TextPosition position = selection.normalized().start();
	std::string filePath = doc.getFilePath();
	if ( filePath.empty() )
		filePath = doc.getLoadingFilePath();
	const std::string filename = FileSystem::fileNameFromPath( filePath );
	std::string workspaceFolder = getPluginContext() ? getPluginContext()->getCurrentProject() : "";
	std::string relativeFilePath( filePath );
	if ( !workspaceFolder.empty() )
		FileSystem::filePathRemoveBasePath( workspaceFolder, relativeFilePath );
	FileSystem::dirRemoveSlashAtEnd( workspaceFolder );
	std::string workspaceName( FileSystem::fileNameFromPath( workspaceFolder ) );

	const auto milliseconds = Sys::getSystemTime();
	const auto nowTime = static_cast<std::time_t>( Sys::getUnixTimestamp() );
	const std::tm* localTimePtr = std::localtime( &nowTime );
	const std::tm localTime = localTimePtr ? *localTimePtr : std::tm{};
	std::string timezoneOffset = formatSnippetTime( localTime, "%z" );
	if ( timezoneOffset.size() == 5 )
		timezoneOffset.insert( 3, ":" );

	const auto& syntax = doc.getSyntaxDefinition();
	const auto& blockComment = syntax.getBlockComment();
	SnippetParser::VariableMap variables{
		{ "TM_SELECTED_TEXT", doc.getText( selection ).toUtf8() },
		{ "TM_CURRENT_LINE", doc.getLineTextWithoutNewLine( position.line() ).toUtf8() },
		{ "TM_CURRENT_WORD", doc.getWordInPosition( position ).toUtf8() },
		{ "TM_LINE_INDEX", String::toString( position.line() ) },
		{ "TM_LINE_NUMBER", String::toString( position.line() + 1 ) },
		{ "TM_FILENAME", filename },
		{ "TM_FILENAME_BASE", FileSystem::fileRemoveExtension( filename ) },
		{ "TM_DIRECTORY", FileSystem::fileRemoveFileName( filePath ) },
		{ "TM_FILEPATH", filePath },
		{ "RELATIVE_FILEPATH", relativeFilePath },
		{ "WORKSPACE_NAME", workspaceName },
		{ "WORKSPACE_FOLDER", workspaceFolder },
		{ "CLIPBOARD", getUISceneNode() && getUISceneNode()->getWindow()
						   ? getUISceneNode()->getWindow()->getClipboard()->getText()
						   : "" },
		{ "CURSOR_INDEX", String::toString( static_cast<Uint64>( cursorIndex ) ) },
		{ "CURSOR_NUMBER", String::toString( static_cast<Uint64>( cursorIndex + 1 ) ) },
		{ "CURRENT_YEAR", formatSnippetTime( localTime, "%Y" ) },
		{ "CURRENT_YEAR_SHORT", formatSnippetTime( localTime, "%y" ) },
		{ "CURRENT_MONTH", formatSnippetTime( localTime, "%m" ) },
		{ "CURRENT_MONTH_NAME", formatSnippetTime( localTime, "%B" ) },
		{ "CURRENT_MONTH_NAME_SHORT", formatSnippetTime( localTime, "%b" ) },
		{ "CURRENT_DATE", formatSnippetTime( localTime, "%d" ) },
		{ "CURRENT_DAY_NAME", formatSnippetTime( localTime, "%A" ) },
		{ "CURRENT_DAY_NAME_SHORT", formatSnippetTime( localTime, "%a" ) },
		{ "CURRENT_HOUR", formatSnippetTime( localTime, "%H" ) },
		{ "CURRENT_MINUTE", formatSnippetTime( localTime, "%M" ) },
		{ "CURRENT_SECOND", formatSnippetTime( localTime, "%S" ) },
		{ "CURRENT_MILLISECOND",
		  String::format( "%03d", static_cast<int>( milliseconds % 1000 ) ) },
		{ "CURRENT_SECONDS_UNIX", String::toString( static_cast<Int64>( milliseconds / 1000 ) ) },
		{ "CURRENT_MILLISECONDS_UNIX", String::toString( static_cast<Int64>( milliseconds ) ) },
		{ "CURRENT_TIMEZONE_OFFSET", timezoneOffset },
		{ "CURRENT_TIMEZONE_NAME", formatSnippetTime( localTime, "%Z" ) },
		{ "RANDOM", String::format( "%06d", Math::randi( 0, 999999 ) ) },
		{ "RANDOM_HEX", String::format( "%06x", Math::randi( 0, 0xFFFFFF ) ) },
		{ "UUID", UUID().toString() },
		{ "LINE_COMMENT", syntax.getComment() },
		{ "BLOCK_COMMENT_START", blockComment.open },
		{ "BLOCK_COMMENT_END", blockComment.close },
	};
	return variables;
}

static std::string prepareSnippetText( TextDocument& doc, const TextRange& selection,
									   std::string_view snippet ) {
	if ( snippet.find( '\n' ) == std::string_view::npos &&
		 snippet.find( '\t' ) == std::string_view::npos )
		return std::string( snippet );
	const TextPosition position = selection.normalized().start();
	const TextPosition contentStart = doc.startOfContent( position );
	std::string baseIndent;
	if ( contentStart.column() > 0 )
		baseIndent =
			doc.line( position.line() ).getText().substr( 0, contentStart.column() ).toUtf8();
	const std::string indent = doc.getIndentString().toUtf8();
	std::string prepared;
	prepared.reserve( snippet.size() + baseIndent.size() * 2 );
	bool lineStart = true;
	for ( const char ch : snippet ) {
		if ( lineStart && ch == '\t' ) {
			prepared += indent;
			continue;
		}
		prepared.push_back( ch );
		lineStart = ch == '\n';
		if ( lineStart )
			prepared += baseIndent;
	}
	return prepared;
}

static TextRange userSnippetActivationRange( TextDocument& doc, const TextRange& selection,
											 std::string_view prefix,
											 std::string_view partialSymbol ) {
	if ( selection.hasSelection() )
		return selection;
	const TextPosition end = selection.start();
	const auto rangeFor = [&]( std::string_view text ) {
		return TextRange(
			doc.positionOffset( end, -static_cast<int>( String::utf8Length( text ) ) ), end );
	};
	if ( !prefix.empty() ) {
		const TextRange prefixRange = rangeFor( prefix );
		if ( doc.getText( prefixRange ).toUtf8() == prefix )
			return prefixRange;
	}
	if ( !partialSymbol.empty() ) {
		const TextRange symbolRange = rangeFor( partialSymbol );
		if ( doc.getText( symbolRange ).toUtf8() == partialSymbol )
			return symbolRange;
	}
	return selection;
}

void AutoCompletePlugin::insertSnippet( UICodeEditor* editor, std::string_view body,
										const Suggestion* suggestion ) {
	auto doc = editor->getDocumentRef();
	auto prevSels = doc->getSelections();
	std::vector<SnippetInsertion> insertions;
	insertions.reserve( prevSels.size() );
	for ( size_t index = 0; index < prevSels.size(); ++index ) {
		const auto& selection = prevSels[index];
		const std::string prepared = prepareSnippetText( *doc, selection, body );
		insertions.push_back(
			{ SnippetParser::parse( prepared, snippetVariables( *doc, selection, index ) ), {} } );
	}

	if ( suggestion && suggestion->source == Suggestion::Source::UserSnippet ) {
		const std::string symbol( getPartialSymbol( doc.get() ) );
		for ( size_t index = 0; index < prevSels.size(); ++index )
			doc->setSelection( index,
							   userSnippetActivationRange( *doc, prevSels[index],
														   suggestion->matchedPrefix, symbol ) );
	} else if ( suggestion && prevSels.size() == 1 && suggestion->range.isValid() &&
				doc->isValidRange( suggestion->range ) ) {
		doc->setSelection( suggestion->range );
	} else if ( suggestion ) {
		const std::string symbol( getPartialSymbol( doc.get() ) );
		if ( !symbol.empty() )
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
		insertSnippet( editor, rawInsertText, &suggestion );
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
	if ( !mSuggestionsEditor )
		return {};
	UICodeEditor* editor = nullptr;
	{
		Lock l( mSuggestionsEditorMutex );
		editor = mSuggestionsEditor;
	}
	if ( !editor )
		return {};
	std::string symbol( getPartialSymbol( editor->getDocumentRef().get() ) );
	SymbolsList userSnippets =
		getUserSnippetSuggestions( editor, symbol, eemax<size_t>( 100UL, suggestions.size() ) );
	const std::string& lang = editor->getDocument().getSyntaxDefinition().getLanguageName();
	bool hasLangSuggestions = false;
	{
		Lock l2( mLangSymbolsMutex );
		auto langSuggestions = mLangCache.find( lang );
		hasLangSuggestions = langSuggestions != mLangCache.end();
	}
	if ( symbol.empty() ) {
		suggestions.insert( suggestions.end(), std::make_move_iterator( userSnippets.begin() ),
							std::make_move_iterator( userSnippets.end() ) );
		Lock l( mSuggestionsMutex );
		mSuggestions = std::move( suggestions );
	} else {
		SymbolsList fuzzySuggestions;
		if ( hasLangSuggestions ) {
			Lock l2( mLangSymbolsMutex );
			auto& symbols = mLangCache[lang];
			fuzzySuggestions = fuzzyMatchSymbols( { &suggestions, &symbols, &userSnippets }, symbol,
												  eemax<size_t>( 100UL, suggestions.size() ) );
		} else {
			fuzzySuggestions = fuzzyMatchSymbols( { &suggestions, &userSnippets }, symbol,
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

	mShortcuts.closeSuggestion = toShortcut( mKeyBindings["autocomplete-close-suggestion"] );
	mShortcuts.prevSuggestion = toShortcut( mKeyBindings["autocomplete-prev-suggestion"] );
	mShortcuts.nextSuggestion = toShortcut( mKeyBindings["autocomplete-next-suggestion"] );
	mShortcuts.firstSuggestion = toShortcut( mKeyBindings["autocomplete-first-suggestion"] );
	mShortcuts.lastSuggestion = toShortcut( mKeyBindings["autocomplete-last-suggestion"] );
	mShortcuts.prevSuggestionPage = toShortcut( mKeyBindings["autocomplete-prev-suggestion-page"] );
	mShortcuts.nextSuggestionPage = toShortcut( mKeyBindings["autocomplete-next-suggestion-page"] );
	mShortcuts.pickSuggestion = toShortcut( mKeyBindings["autocomplete-pick-suggestion"] );
	mShortcuts.pickSuggestionAlt = toShortcut( mKeyBindings["autocomplete-pick-suggestion-alt"] );
	mShortcuts.pickSuggestionAlt2 =
		toShortcut( mKeyBindings["autocomplete-pick-suggestion-alt-2"] );
	mShortcuts.updateSuggestions = toShortcut( mKeyBindings["autocomplete-update-suggestions"] );
	mShortcuts.closeSignatureHelp = toShortcut( mKeyBindings["autocomplete-close-signature-help"] );
	mShortcuts.prevSignatureHelp = toShortcut( mKeyBindings["autocomplete-prev-signature-help"] );
	mShortcuts.nextSignatureHelp = toShortcut( mKeyBindings["autocomplete-next-signature-help"] );
}

PluginRequestHandle AutoCompletePlugin::processResponse( const PluginMessage& msg ) {
	if ( msg.type == PluginMessageType::UIReady ) {
		updateShortcuts();
	} else if ( msg.type == PluginMessageType::WorkspaceFolderChanged ) {
		setSnippetWorkspaceFolder( msg.asJSON().value( "folder", "" ) );
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

std::string AutoCompletePlugin::getUserSnippetInput( UICodeEditor* editor ) const {
	if ( !editor || editor->getDocument().getSelection().hasSelection() )
		return {};
	TextDocument& doc = editor->getDocument();
	const TextPosition end = doc.getSelection().end();
	static constexpr size_t MAX_SNIPPET_INPUT_LENGTH = 128;
	const TextPosition start(
		end.line(),
		eemax<Int64>( 0, end.column() - static_cast<Int64>( MAX_SNIPPET_INPUT_LENGTH ) ) );
	std::string input = doc.getText( { start, end } ).toUtf8();
	const size_t whitespace = input.find_last_of( " \t" );
	if ( whitespace != std::string::npos )
		input.erase( 0, whitespace + 1 );
	return input;
}

void AutoCompletePlugin::update( UICodeEditor* editor ) {
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
				auto docRef = getPluginContext()->getSplitter()->getTextDocumentRef( doc );
				if ( !docRef )
					continue;
				{
					Lock lu( mDocsUpdatingMutex );
					auto& updating = mDocsUpdating[doc];
					// Don't queue another cache update while one is queued or running.
					if ( updating )
						continue;
					updating = true;
				}
				mThreadPool->run( [this, doc = std::move( docRef )]() mutable {
					updateDocCache( std::move( doc ) );
				} );
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

		Drawable* icon = nullptr;
		UIIcon* iconSource = editor->getUISceneNode()->findIcon(
			LSPCompletionItemHelper::toIconString( suggestion.kind ) );
		if ( iconSource )
			icon = iconSource->getSource( PixelDensity::dpToPxI( 12 ) ).get();

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

AutoCompletePlugin::SymbolsList
AutoCompletePlugin::getDocumentSymbols( const std::shared_ptr<TextDocument>& docRef ) {
	static constexpr auto MAX_LINE_COUNT = EE_1KB * 10;
	AutoCompletePlugin::SymbolsList symbols;
	if ( !docRef )
		return symbols;
	TextDocument* doc = docRef.get();
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
		{
			Lock l( mDocMutex );
			if ( mShuttingDown || mDocs.find( doc ) == mDocs.end() )
				break;
		}
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
		SymbolsList userSnippets =
			getUserSnippetSuggestions( editor, symbol, mSuggestionsMaxVisible );

		SymbolsList matches;
		if ( symbol.empty() ) {
			matches = std::move( userSnippets );
		} else {
			Lock l( fromDocCache ? mDocMutex : mLangSymbolsMutex );
			matches =
				fuzzyMatchSymbols( { &symbols, &userSnippets }, symbol, mSuggestionsMaxVisible );
		}
		Lock l2( mSuggestionsMutex );
		mSuggestions = std::move( matches );
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

	bool scheduled = false;
	if ( usesOwnSymbols ) {
		Lock l( mDocMutex );
		auto docCache = mDocCache.find( &doc );
		if ( docCache != mDocCache.end() && !mShuttingDown ) {
			const auto& symbols = docCache->second.symbols;
			mThreadPool->run( [this, symbol, &symbols, editor] {
				runUpdateSuggestions( symbol, symbols, editor, true );
			} );
			scheduled = true;
		}
	}

	{
		const std::string& lang = doc.getSyntaxDefinition().getLanguageName();
		Lock l( mLangSymbolsMutex );
		auto langSuggestions = mLangCache.find( lang );
		if ( langSuggestions != mLangCache.end() ) {
			const auto& symbols = langSuggestions->second;
			mThreadPool->run( [this, symbol, &symbols, editor] {
				runUpdateSuggestions( symbol, symbols, editor, false );
			} );
			scheduled = true;
		}
	}
	if ( !scheduled )
		mThreadPool->run( [this, symbol, editor] {
			runUpdateSuggestions( symbol, SymbolsList{}, editor, false );
		} );
}

AutoCompletePlugin::SymbolsList
AutoCompletePlugin::getUserSnippetSuggestions( UICodeEditor* editor, const std::string& symbol,
											   size_t maxResults ) const {
	SymbolsList suggestions;
	if ( !editor )
		return suggestions;
	const auto& language = editor->getDocument().getSyntaxDefinition().getLSPName();
	std::string snippetInput = getUserSnippetInput( editor );
	if ( snippetInput.empty() )
		snippetInput = symbol;
	std::string filePath = editor->getDocument().getFilePath();
	if ( getPluginContext() )
		FileSystem::filePathRemoveBasePath( getPluginContext()->getCurrentProject(), filePath );
	auto matches = mUserSnippetStore.find( language, snippetInput, maxResults, filePath );
	suggestions.reserve( matches.size() );
	for ( auto& match : matches ) {
		Suggestion suggestion( LSPCompletionItemKind::Snippet, std::move( match.matchedPrefix ),
							   match.snippet.description.empty()
								   ? std::move( match.snippet.name )
								   : match.snippet.name + " - " + match.snippet.description,
							   {}, {}, std::move( match.snippet.body ), {},
							   LSPInsertTextFormat::Snippet );
		suggestion.source = Suggestion::Source::UserSnippet;
		suggestion.matchedPrefix = std::move( match.matchedInput );
		suggestion.identityHash = hashCombine( String::hash( match.snippet.sourcePath ),
											   String::hash( match.snippet.name ) );
		suggestion.score = match.score;
		suggestion.sourcePriority = match.snippet.source == UserSnippetSource::EcodeProject	   ? 2
									: match.snippet.source == UserSnippetSource::VSCodeProject ? 1
																							   : 0;
		suggestions.emplace_back( std::move( suggestion ) );
	}
	return suggestions;
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
