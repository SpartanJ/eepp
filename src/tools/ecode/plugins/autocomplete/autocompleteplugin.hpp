#ifndef ECODE_AUTOCOMPLETEPLUGIN_HPP
#define ECODE_AUTOCOMPLETEPLUGIN_HPP

#include "../lsp/lspprotocol.hpp"
#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "snippetparser.hpp"
#include "usersnippetstore.hpp"
#include <eepp/config.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <unordered_map>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class AutoCompletePlugin : public Plugin {
  public:
	class Suggestion {
	  public:
		enum class Source { LocalSymbol, LSP, UserSnippet, SnippetChoice };

		LSPCompletionItemKind kind{ LSPCompletionItemKind::Text };
		std::string text;
		std::string detail;
		std::string sortText;
		TextRange range;
		std::string insertText;
		LSPInsertTextFormat insertTextFormat{ LSPInsertTextFormat::PlainText };
		double score{ 0 };
		int sourcePriority{ 0 };
		LSPMarkupContent documentation;
		size_t identityHash{ 0 };
		std::string matchedPrefix;
		Source source{ Source::LocalSymbol };

		void setScore( const double& score ) const {
			const_cast<Suggestion*>( this )->score = score;
		}

		Suggestion( const std::string& text ) : text( text ), sortText( text ) {}

		Suggestion( LSPCompletionItemKind kind, std::string&& text, std::string&& detail,
					std::string&& sortText, const TextRange& range, std::string&& insertText,
					LSPMarkupContent&& doc, LSPInsertTextFormat insertTextFormat ) :
			kind( kind ),
			text( std::move( text ) ),
			detail( std::move( detail ) ),
			sortText( sortText.empty() ? std::string{ this->text } : std::move( sortText ) ),
			range( range ),
			insertText( std::move( insertText ) ),
			insertTextFormat( insertTextFormat ),
			documentation( std::move( doc ) ),
			source( Source::LSP ) {};

		bool operator<( const Suggestion& other ) const { return getCmpStr() < other.getCmpStr(); }

		bool operator==( const Suggestion& other ) const {
			if ( source == Source::UserSnippet || other.source == Source::UserSnippet )
				return source == other.source && identityHash == other.identityHash;
			return text == other.text;
		}

	  protected:
		const std::string* getCmpStr() const { return !sortText.empty() ? &sortText : &text; }
	};
	typedef std::vector<Suggestion> SymbolsList;

	static PluginDefinition Definition() {
		return { "autocomplete",
				 "Auto Complete",
				 "Auto complete shows the completion popup as you type, so you can fill "
				 "in long words by typing only a few characters.",
				 AutoCompletePlugin::New,
				 { 0, 3, 0 },
				 AutoCompletePlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~AutoCompletePlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	bool isReady() const override { return true; }

	bool hasSettingsPage() const { return true; }

	void registerSettings( SettingsPage& page );

	void onRegister( UICodeEditor* ) override;

	void onUnregister( UICodeEditor* ) override;

	bool onKeyDown( UICodeEditor*, const KeyEvent& ) override;

	bool onTextInput( UICodeEditor*, const TextInputEvent& ) override;

	void update( UICodeEditor* ) override;

	void postDraw( UICodeEditor*, const Vector2f& startScroll, const Float& lineHeight,
				   const TextPosition& cursor ) override;

	void drawBeforeLineText( UICodeEditor*, const Int64&, Vector2f, const Float&,
							 const Float& ) override;

	bool onMouseDown( UICodeEditor*, const Vector2i&, const Uint32& ) override;

	bool onMouseUp( UICodeEditor*, const Vector2i&, const Uint32& ) override;

	bool onMouseDoubleClick( UICodeEditor*, const Vector2i&, const Uint32& ) override;

	bool onMouseMove( UICodeEditor*, const Vector2i&, const Uint32& ) override;

	void onFileSystemEvent( const FileEvent&, const FileInfo& ) override;

	FileSystemListenerOptions getFileSystemListenerOptions() const override;

	void onLoadProject( const std::string& projectFolder,
						const std::string& projectStatePath ) override;

	const Rectf& getBoxPadding() const;

	void setBoxPadding( const Rectf& boxPadding );

	const Int32& getSuggestionsMaxVisible() const;

	void setSuggestionsMaxVisible( const Uint32& suggestionsMaxVisible );

	const Time& getUpdateFreq() const;

	void setUpdateFreq( const Time& updateFreq );

	const std::string& getSymbolPattern() const;

	void setSymbolPattern( const std::string& symbolPattern );

	bool isDirty() const;

	void setDirty( bool dirty );

	bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu, const Vector2i& position,
							  const Uint32& flags ) override;

  protected:
	std::string mSymbolPattern;
	Rectf mBoxPadding;
	Clock mClock;
	Mutex mLangSymbolsMutex;
	Mutex mSuggestionsMutex;
	Mutex mDocMutex;
	Time mUpdateFreq{ Seconds( 5 ) };
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::unordered_set<TextDocument*> mDocs;
	std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
	bool mDirty{ false };
	bool mReplacing{ false };
	bool mSignatureHelpVisible{ false };
	bool mHighlightSuggestions{ true };
	struct DocCache {
		Uint64 changeId{ static_cast<Uint64>( -1 ) };
		SymbolsList symbols;
	};
	std::unordered_map<TextDocument*, DocCache> mDocCache;
	std::unordered_map<TextDocument*, bool> mDocUsesOwnSymbols;
	std::unordered_map<std::string, SymbolsList> mLangCache;
	std::vector<Suggestion> mSuggestions;
	Mutex mSuggestionsEditorMutex;
	Mutex mSignatureHelpEditorMutex;
	UICodeEditor* mSuggestionsEditor{ nullptr };
	UICodeEditor* mSignatureHelpEditor{ nullptr };
	Int32 mSuggestionIndex{ 0 };
	Int32 mSuggestionsMaxVisible{ 8 };
	Int32 mSuggestionsStartIndex{ 0 };
	std::unordered_map<std::string, LSPServerCapabilities> mCapabilities;
	Mutex mCapabilitiesMutex;
	Mutex mDocUsesOwnSymbolsMutex;

	struct SignatureInformation {
		String label;
		LSPMarkupContent documentation;
		std::vector<TextSelectionRange> parameters;
	};

	struct SignatureHelp {
		std::vector<SignatureInformation> signatures;
		int activeSignature{ 0 };
		int activeParameter{ 0 };
	};

	SignatureHelp mSignatureHelp;
	TextPosition mSignatureHelpPosition;
	Int32 mSignatureHelpSelected{ -1 };
	Mutex mHandlesMutex;
	std::unordered_map<TextDocument*, std::vector<PluginIDType>> mHandles;
	std::unordered_map<TextDocument*, std::atomic<bool>> mDocsUpdating;
	Mutex mDocsUpdatingMutex;
	Text mSuggestionDoc;
	Text mSignatureHelpText;
	size_t mMaxLabelCharacters{ 100 };
	String::HashType mConfigHash{ 0 };
	std::unordered_map<std::string, std::string> mKeyBindings;
	struct Shortcuts {
		KeyBindings::Shortcut closeSuggestion;
		KeyBindings::Shortcut prevSuggestion;
		KeyBindings::Shortcut nextSuggestion;
		KeyBindings::Shortcut firstSuggestion;
		KeyBindings::Shortcut lastSuggestion;
		KeyBindings::Shortcut prevSuggestionPage;
		KeyBindings::Shortcut nextSuggestionPage;
		KeyBindings::Shortcut pickSuggestion;
		KeyBindings::Shortcut pickSuggestionAlt;
		KeyBindings::Shortcut pickSuggestionAlt2;
		KeyBindings::Shortcut updateSuggestions;
		KeyBindings::Shortcut closeSignatureHelp;
		KeyBindings::Shortcut prevSignatureHelp;
		KeyBindings::Shortcut nextSignatureHelp;
	} mShortcuts;
	std::string mMaxSuggestionDocumentationWidth{ "100%" };
	std::string mMaxSignatureHelperWidth{ "90%" };
	bool mSignatureHelpMultiLine{ true };
	bool mSuggestionDocumentation{ true };
	bool mSignatureHelpDocumentation{ true };
	bool mLoadVSCodeSnippets{ true };

	Float mRowHeight{ 0 };
	Rectf mBoxRect;

	struct SnippetTabStopOccurrence {
		TextRange range;
		size_t instance{ 0 };
		std::vector<std::string> choices;
	};

	struct SnippetTabStopGroup {
		Uint32 index{ 0 };
		std::vector<SnippetTabStopOccurrence> occurrences;
	};

	struct SnippetSession {
		UICodeEditor* editor{ nullptr };
		std::vector<SnippetTabStopGroup> groups;
		size_t currentGroup{ 0 };
		size_t instanceCount{ 0 };
	};

	struct SnippetInsertion {
		SnippetParser::Result snippet;
		TextPosition start;
	};

	class SnippetDocumentClient;
	UnorderedMap<TextDocument*, SnippetSession> mSnippetSessions;
	UnorderedMap<TextDocument*, std::unique_ptr<SnippetDocumentClient>> mSnippetClients;
	bool mChangingSnippetSelection{ false };
	bool mSnippetChoiceSuggestions{ false };
	UserSnippetStore mUserSnippetStore;
	Mutex mSnippetLoadMutex;
	std::string mUserSnippetsPath;
	std::string mSnippetWorkspaceFolder;
	std::string mVSCodeSnippetsPath;
	std::string mEcodeSnippetsPath;
	std::string mSnippetEventPathBuffer;
	std::atomic<Uint64> mSnippetWorkspaceGeneration{ 0 };
	std::atomic<Uint32> mSnippetJobs{ 0 };
	Uint64 mSnippetLocatorProviderId{ 0 };

	explicit AutoCompletePlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	void resetSuggestions( UICodeEditor* editor );

	void updateSuggestions( const std::string& symbol, UICodeEditor* editor );

	SymbolsList getDocumentSymbols( const std::shared_ptr<TextDocument>& doc );

	void updateDocCache( std::shared_ptr<TextDocument> doc );

	std::string getPartialSymbol( TextDocument* doc );

	void runUpdateSuggestions( const std::string& symbol, const SymbolsList& symbols,
							   UICodeEditor* editor, bool fromDocCache );

	SymbolsList getUserSnippetSuggestions( UICodeEditor* editor, const std::string& symbol,
										   size_t maxResults ) const;

	std::string getUserSnippetInput( UICodeEditor* editor ) const;

	void loadSnippetDirectory( const std::string& path, UserSnippetSource source,
							   bool languageFiles );

	void loadSnippetFile( const std::string& path, UserSnippetSource source, bool languageFiles );

	void setSnippetWorkspaceFolder( std::string workspaceFolder );

	void scheduleSnippetFileUpdate( std::string path, UserSnippetSource source, bool languageFiles,
									bool remove );

	void registerSnippetLocatorProvider();

	void unregisterSnippetLocatorProvider();

	void insertSnippet( UICodeEditor* editor, std::string_view body,
						const Suggestion* suggestion = nullptr );

	SnippetParser::VariableMap snippetVariables( TextDocument& doc, const TextRange& selection,
												 size_t cursorIndex ) const;

	void updateLangCache( const std::string& langName );

	void pickSuggestion( UICodeEditor* editor );

	PluginRequestHandle processResponse( const PluginMessage& msg );

	bool tryRequestCapabilities( UICodeEditor* editor );

	void requestCodeCompletion( UICodeEditor* editor );

	void requestSignatureHelp( UICodeEditor* editor );

	PluginRequestHandle processCodeCompletion( const LSPCompletionList& completion );

	PluginRequestHandle processSignatureHelp( const LSPSignatureHelp& signatureHelp );

	void resetSignatureHelp();

	void drawSignatureHelp( UICodeEditor* editor, const Vector2f& startScroll,
							const Float& lineHeight, bool drawUp );

	void tryStartSnippetNav( const std::vector<SnippetInsertion>&, UICodeEditor* );

	void ensureSnippetClient( TextDocument* );

	void detachSnippetClient( TextDocument* );

	void cancelSnippetSession( TextDocument*, bool collapseSelection = false );

	bool navigateSnippet( UICodeEditor*, bool backwards );

	void selectSnippetGroup( SnippetSession&, bool showChoices = true );

	void showSnippetChoices( SnippetSession& );

	void pickSnippetChoice( UICodeEditor* );

	void onSnippetTextChanged( TextDocument*, const DocumentContentChange& );

	void onSnippetSelectionChanged( TextDocument* );

	void onSnippetDocumentClosed( TextDocument* );

	Rectf findBestDocumentationPlacement( UICodeEditor* editor, const LSPMarkupContent& suggestion,
										  const std::string& detail, const Rectf& anchorBox,
										  const Rectf& rowRect, const Vector2f& cursorScreenPos,
										  bool drawUp, Float lineHeight );

	void updateShortcuts();
};

} // namespace ecode

#endif // ECODE_AUTOCOMPLETEPLUGIN_HPP
