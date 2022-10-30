#ifndef ECODE_LSPDOCUMENTCLIENT_HPP
#define ECODE_LSPDOCUMENTCLIENT_HPP

#include <eepp/system/clock.hpp>
#include <eepp/ui/doc/textdocument.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI::Doc;

namespace ecode {

class LSPClientServer;

class LSPDocumentClient : public TextDocument::Client {
  public:
	LSPDocumentClient( LSPClientServer* server, TextDocument* doc );

	virtual void onDocumentTextChanged();
	virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& eventType );
	virtual void onDocumentCursorChange( const TextPosition& );
	virtual void onDocumentSelectionChange( const TextRange& );
	virtual void onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount );
	virtual void onDocumentLineChanged( const Int64& lineIndex );
	virtual void onDocumentSaved( TextDocument* );
	virtual void onDocumentClosed( TextDocument* );
	virtual void onDocumentDirtyOnFileSystem( TextDocument* );
	virtual void onDocumentMoved( TextDocument* );

	bool isDirty() const;

	void notifyOpen();

	void resetDirty();

	TextDocument* getDoc() const;

	LSPClientServer* getServer() const;

	int getVersion() const;

  protected:
	LSPClientServer* mServer{ nullptr };
	TextDocument* mDoc{ nullptr };
	bool mModified{ false };
	int mVersion{ 0 };
	Clock mLastModified;
};

} // namespace ecode

#endif // ECODE_LSPDOCUMENTCLIENT_HPP
