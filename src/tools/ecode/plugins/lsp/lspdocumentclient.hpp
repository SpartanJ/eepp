#ifndef ECODE_LSPDOCUMENTCLIENT_HPP
#define ECODE_LSPDOCUMENTCLIENT_HPP

#include "lspprotocol.hpp"
#include <eepp/system/clock.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/doc/textdocument.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Doc;

namespace EE { namespace UI {
class UISceneNode;
}} // namespace EE::UI

namespace ecode {

class LSPClientServer;
class LSPClientServerManager;

class LSPDocumentClient : public TextDocument::Client {
  public:
	LSPDocumentClient( LSPClientServer* server, TextDocument* doc );

	~LSPDocumentClient();

	virtual void onDocumentLoaded( TextDocument* );
	virtual void onDocumentTextChanged( const DocumentContentChange& change );
	virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& eventType );
	virtual void onDocumentCursorChange( const TextPosition& );
	virtual void onDocumentSelectionChange( const TextRange& );
	virtual void onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount );
	virtual void onDocumentLineChanged( const Int64& lineIndex );
	virtual void onDocumentSaved( TextDocument* );
	virtual void onDocumentClosed( TextDocument* );
	virtual void onDocumentDirtyOnFileSystem( TextDocument* );
	virtual void onDocumentMoved( TextDocument* );
	virtual void onDocumentReloaded( TextDocument* );
	virtual void onDocumentReset( TextDocument* );

	void notifyOpen();

	TextDocument* getDoc() const;

	LSPClientServer* getServer() const;

	int getVersion() const;

	void onServerInitialized();

	void requestSymbols();

	void requestSymbolsDelayed();

	void requestSemanticHighlighting( bool reqFull = false );

	void requestSemanticHighlightingDelayed( bool reqFull = false );

	void requestFoldRange();

	bool isRunningSemanticTokens() const;

	bool isWaitingSemanticTokensResponse() const;

	void requestCodeLens();

  protected:
	LSPClientServer* mServer{ nullptr };
	LSPClientServerManager* mServerManager{ nullptr };
	TextDocument* mDoc{ nullptr };
	String::HashType mTag{ 0 };
	String::HashType mTagSemanticTokens{ 0 };
	int mVersion{ 0 };
	std::string mSemanticeResultId;
	LSPSemanticTokensDelta mSemanticTokens;
	std::vector<LSPCodeLens> mCodeLens;
	bool mRunningSemanticTokens{ false };
	bool mWaitingSemanticTokensResponse{ false };
	bool mShutdown{ false };
	bool mFirstHighlight{ true };

	void refreshTag();

	UISceneNode* getUISceneNode();

	void processTokens( LSPSemanticTokensDelta&& tokens, const Uint64& docModificationId );

	void highlight();
};

} // namespace ecode

#endif // ECODE_LSPDOCUMENTCLIENT_HPP
