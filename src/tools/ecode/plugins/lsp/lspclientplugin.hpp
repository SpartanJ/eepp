#ifndef ECODE_LSPPLUGIN_HPP
#define ECODE_LSPPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "lspclientservermanager.hpp"
#include <eepp/config.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilistview.hpp>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class LSPSymbolInfoTreeModel;

// Implementation of the LSP Client:
// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
class LSPClientPlugin : public Plugin {
  public:
	static PluginDefinition Definition() {
		return { "lspclient",		   "LSP Client", "Language Server Protocol Client.",
				 LSPClientPlugin::New, { 0, 2, 9 },	 LSPClientPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~LSPClientPlugin();

	virtual void update( UICodeEditor* );

	std::string getId() { return Definition().id; }

	std::string getTitle() { return Definition().name; }

	std::string getDescription() { return Definition().description; }

	virtual String::HashType getConfigFileHash() { return mConfigHash; }

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

	const UnorderedMap<UICodeEditor*, TextDocument*>& getEditorDocs() { return mEditorDocs; };

	virtual bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
									  const Vector2i& position, const Uint32& flags );

	virtual bool onMouseMove( UICodeEditor* editor, const Vector2i& position, const Uint32& flags );

	virtual bool onMouseLeave( UICodeEditor* editor, const Vector2i& position,
							   const Uint32& flags );

	virtual void onFocusLoss( UICodeEditor* editor );

	virtual bool onKeyDown( UICodeEditor*, const KeyEvent& );

	const Time& getHoverDelay() const;

	void setHoverDelay( const Time& hoverDelay );

	const LSPClientServerManager& getClientManager() const;

	bool processDocumentFormattingResponse( const URI& uri, std::vector<LSPTextEdit> edits );

	const LSPSymbolInformationList& getDocumentSymbols( TextDocument* doc );

	const LSPSymbolInformationList& getDocumentSymbols( const URI& uri );

	const LSPSymbolInformationList& getDocumentFlattenSymbols( const URI& uri );

	TextDocument* getDocumentFromURI( const URI& uri );

	virtual bool onMouseClick( UICodeEditor* editor, const Vector2i& pos, const Uint32& flags );

	bool semanticHighlightingEnabled() const;

	void setSemanticHighlighting( bool semanticHighlighting );

	bool langSupportsSemanticHighlighting( const std::string& lspLang );

	bool isSilent() const;

	void setSilent( bool silence = true );

	bool trimLogs() const;

	void setTrimLogs( bool trimLogs );

	void onVersionUpgrade( Uint32 oldVersion, Uint32 currentVersion );

	void drawTop( UICodeEditor* editor, const Vector2f& screenStart, const Sizef& size,
				  const Float& fontSize );

  protected:
	friend class LSPDocumentClient;
	friend class LSPClientServer;

	Clock mClock;
	Mutex mDocMutex;
	Mutex mDocSymbolsMutex;
	Mutex mDocCurrentSymbolsMutex;
	UnorderedMap<UICodeEditor*, std::vector<Uint32>> mEditors;
	UnorderedSet<TextDocument*> mDocs;
	UnorderedMap<URI, LSPSymbolInformationList> mDocSymbols;
	UnorderedMap<URI, LSPSymbolInformationList> mDocFlatSymbols;
	UnorderedMap<UICodeEditor*, TextDocument*> mEditorDocs;
	LSPClientServerManager mClientManager;
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	bool mOldWordWrap{ false };
	bool mSymbolInfoShowing{ false };
	bool mSemanticHighlighting{ true };
	bool mSilence{ false };
	bool mTrimLogs{ false };
	bool mBreadcrumb{ true };
	UICodeEditor* mHoveringBreadcrumb{ nullptr };
	StyleSheetLength mBreadcrumbHeight{ "20dp" };
	std::unordered_map<std::string, std::string> mKeyBindings; /* cmd, shortcut */
	UnorderedMap<TextDocument*, std::shared_ptr<TextDocument>> mDelayedDocs;
	Uint32 mHoverWaitCb{ 0 };
	LSPHover mCurrentHover;
	Time mHoverDelay{ Seconds( 1.f ) };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };
	LSPDiagnosticsCodeAction mQuickFix;
	UnorderedSet<std::string> mSemanticHighlightingDisabledLangs;
	String::HashType mConfigHash{ 0 };
	Color mOldBackgroundColor;
	std::string mOldMaxWidth;
	Float mPluginTopSpace{ 0 };
	struct DisplaySymbolInfo {
		String name;
		std::string icon;

		bool operator==( const DisplaySymbolInfo& other ) const {
			return name == other.name && icon == other.icon;
		}
	};
	UnorderedMap<URI, std::vector<DisplaySymbolInfo>> mDocCurrentSymbols;
	UIIcon* mDrawSepIcon{ nullptr };

	LSPClientPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	void loadLSPConfig( std::vector<LSPDefinition>& lsps, const std::string& path,
						bool updateConfigFile );

	size_t lspFilePatternPosition( const std::vector<LSPDefinition>& lsps,
								   const std::vector<std::string>& patterns );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	PluginRequestHandle processCodeCompletionRequest( const PluginMessage& msg );

	PluginRequestHandle processSignatureHelpRequest( const PluginMessage& msg );

	PluginRequestHandle processCancelRequest( const PluginMessage& msg );

	PluginRequestHandle processDocumentFormatting( const PluginMessage& msg );

	PluginRequestHandle processWorkspaceSymbol( const PluginMessage& msg );

	PluginRequestHandle processWorkspaceDiagnostic( const PluginMessage& msg );

	void tryHideTooltip( UICodeEditor* editor, const Vector2i& position );

	void hideTooltip( UICodeEditor* editor );

	void tryDisplayTooltip( UICodeEditor* editor, const LSPHover& resp, const Vector2i& position );

	void displayTooltip( UICodeEditor* editor, const LSPHover& resp, const Vector2f& position );

	void getSymbolInfo( UICodeEditor* editor );

	void switchSourceHeader( UICodeEditor* editor );

	void createLocationsView( UICodeEditor* editor, const std::vector<LSPLocation>& locs );

	void getAndGoToLocation( UICodeEditor* editor, const std::string& search );

	void codeAction( UICodeEditor* editor );

	void createCodeActionsView( UICodeEditor* editor, const std::vector<LSPCodeAction>& cas );

	PluginRequestHandle processTextDocumentSymbol( const PluginMessage& msg );

	PluginRequestHandle processFoldingRanges( const PluginMessage& msg );

	void setDocumentSymbols( const URI& docURI, LSPSymbolInformationList&& res );

	void setDocumentSymbolsFromResponse( const PluginIDType& id, const URI& docURI,
										 LSPSymbolInformationList&& res );

	void processDiagnosticsCodeAction( const PluginMessage& msg );

	void renameSymbol( UICodeEditor* editor );

	void updateCurrentSymbol( TextDocument& doc );

	void showDocumentSymbols( UICodeEditor* editor );

	std::shared_ptr<LSPSymbolInfoTreeModel> createDocSymbolsModel( URI uri,
																   const std::string& query = "" );

	void onDocumentHoverResponse( UICodeEditor* editor, const LSPHover& resp, bool fromMousePos,
								  std::optional<Vector2f> screenPos = {} );
};

} // namespace ecode

#endif // ECODE_LSPPLUGIN_HPP
