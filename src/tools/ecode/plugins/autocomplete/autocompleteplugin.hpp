#ifndef ECODE_AUTOCOMPLETEPLUGIN_HPP
#define ECODE_AUTOCOMPLETEPLUGIN_HPP

#include "../lsp/lspprotocol.hpp"
#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include <eepp/config.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <set>
#include <unordered_map>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class AutoCompletePlugin : public Plugin {
  public:
	class Suggestion {
	  public:
		LSPCompletionItemKind kind{ LSPCompletionItemKind::Text };
		std::string text;
		std::string detail;
		std::string sortText;
		TextRange range;
		double score{ 0 };

		void setScore( const double& score ) const {
			const_cast<Suggestion*>( this )->score = score;
		}

		Suggestion( const std::string& text ) : text( text ), sortText( text ) {}

		Suggestion( const LSPCompletionItemKind& kind, const std::string& text,
					const std::string& detail, const std::string& sortText,
					const TextRange& range = {} ) :
			kind( kind ),
			text( text ),
			detail( detail ),
			sortText( sortText.empty() ? text : sortText ),
			range( range ){};

		bool operator<( const Suggestion& other ) const { return getCmpStr() < other.getCmpStr(); }

		bool operator==( const Suggestion& other ) const { return text == other.text; }

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
				 { 0, 2, 2 } };
	}

	static Plugin* New( PluginManager* pluginManager );

	virtual ~AutoCompletePlugin();

	std::string getId() { return Definition().id; }

	std::string getTitle() { return Definition().name; }

	std::string getDescription() { return Definition().description; }

	bool isReady() const { return true; }

	void onRegister( UICodeEditor* );
	void onUnregister( UICodeEditor* );
	bool onKeyDown( UICodeEditor*, const KeyEvent& );
	bool onTextInput( UICodeEditor*, const TextInputEvent& );
	void update( UICodeEditor* );
	void postDraw( UICodeEditor*, const Vector2f& startScroll, const Float& lineHeight,
				   const TextPosition& cursor );
	bool onMouseDown( UICodeEditor*, const Vector2i&, const Uint32& );
	bool onMouseUp( UICodeEditor*, const Vector2i&, const Uint32& );
	bool onMouseDoubleClick( UICodeEditor*, const Vector2i&, const Uint32& );
	bool onMouseMove( UICodeEditor*, const Vector2i&, const Uint32& );

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

  protected:
	std::string mSymbolPattern;
	Rectf mBoxPadding;
	Clock mClock;
	Mutex mLangSymbolsMutex;
	Mutex mSuggestionsMutex;
	Mutex mDocMutex;
	Time mUpdateFreq{ Seconds( 5 ) };
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::set<TextDocument*> mDocs;
	std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
	bool mDirty{ false };
	bool mReplacing{ false };
	bool mSignatureHelpVisible{ false };
	struct DocCache {
		Uint64 changeId{ static_cast<Uint64>( -1 ) };
		SymbolsList symbols;
	};
	std::unordered_map<TextDocument*, DocCache> mDocCache;
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
	LSPSignatureHelp mSignatureHelp;
	TextPosition mSignatureHelpPosition;
	Int32 mSignatureHelpSelected{ -1 };
	Mutex mHandlesMutex;
	std::unordered_map<TextDocument*, std::vector<PluginIDType>> mHandles;
	std::unordered_map<TextDocument*, std::atomic<bool>> mDocsUpdating;
	Mutex mDocsUpdatingMutex;

	Float mRowHeight{ 0 };
	Rectf mBoxRect;

	explicit AutoCompletePlugin( PluginManager* pluginManager );

	void resetSuggestions( UICodeEditor* editor );

	void updateSuggestions( const std::string& symbol, UICodeEditor* editor );

	SymbolsList getDocumentSymbols( TextDocument* );

	void updateDocCache( TextDocument* doc );

	std::string getPartialSymbol( TextDocument* doc );

	void runUpdateSuggestions( const std::string& symbol, const SymbolsList& symbols,
							   UICodeEditor* editor );

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
};

} // namespace ecode

#endif // ECODE_AUTOCOMPLETEPLUGIN_HPP
