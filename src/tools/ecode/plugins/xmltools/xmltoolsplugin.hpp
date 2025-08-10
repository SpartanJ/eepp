#ifndef ECODE_XMLTOOLSPLUGIN_HPP
#define ECODE_XMLTOOLSPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class XMLToolsPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "xmltools",
				 "XML Tools",
				 "Simple tools to improve your XML editing experience.",
				 XMLToolsPlugin::New,
				 { 0, 0, 4 },
				 XMLToolsPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~XMLToolsPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	bool getHighlightMatch() const;

	bool getAutoEditMatch() const;

	void drawBeforeLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							 const Float& fontSize, const Float& lineHeight ) override;

	void minimapDrawAfter( UICodeEditor*, const DocumentLineRange&, const DocumentViewLineRange&,
						   const Vector2f& /*linePos*/, const Vector2f& /*lineSize*/,
						   const Float& /*charWidth*/, const Float& /*gutterWidth*/,
						   const DrawTextRangesFn& ) override;

  protected:
	bool mHighlightMatch{ true };
	bool mAutoEditMatch{ true };
	Mutex mClientsMutex;

	struct ClientMatch {
		TextRange currentBracket;
		TextRange matchBracket;
		bool currentIsClose;

		bool isSameLine() const {
			return currentBracket.start().line() == matchBracket.start().line();
		}

		TextRange& open() { return currentIsClose ? matchBracket : currentBracket; }

		TextRange& close() { return currentIsClose ? currentBracket : matchBracket; }
	};

	class XMLToolsClient : public TextDocument::Client {
	  public:
		explicit XMLToolsClient( XMLToolsPlugin* parent, TextDocument* doc ) :
			mDoc( doc ), mParent( parent ) {}

		virtual void onDocumentTextChanged( const DocumentContentChange& );
		virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& ) {}
		virtual void onDocumentCursorChange( const TextPosition& ) {}
		virtual void onDocumentInterestingCursorChange( const TextPosition& ) {}
		virtual void onDocumentSelectionChange( const TextRange& );
		virtual void onDocumentLineCountChange( const size_t&, const size_t& ) {}
		virtual void onDocumentLineChanged( const Int64& ) {}
		virtual void onDocumentSaved( TextDocument* ) {}
		virtual void onDocumentClosed( TextDocument* doc ) { onDocumentReset( doc ); }
		virtual void onDocumentDirtyOnFileSystem( TextDocument* ) {}
		virtual void onDocumentMoved( TextDocument* ) {};
		virtual void onDocumentReset( TextDocument* ) { mSelections.clear(); }
		Client::Type getTextDocumentClientType() { return TextDocument::Client::Auxiliary; }

	  protected:
		TextDocument* mDoc{ nullptr };
		XMLToolsPlugin* mParent{ nullptr };
		bool mAutoInserting{ false };
		bool mWaitingText{ false };
		bool mJustDeletedWholeWord{ false };
		int mForceSelections{ 0 };
		TextRanges mSelections;

		void updateMatch( const TextRange& range );

		void clearMatch();

		void updateCurrentMatch( ClientMatch& match, int translation );
	};

	using ClientsMap = std::unordered_map<TextDocument*, std::unique_ptr<XMLToolsClient>>;
	ClientsMap mClients;
	using ClientsMatches = std::unordered_map<TextDocument*, ClientMatch>;
	ClientsMatches mMatches;

	XMLToolsPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	virtual void onRegisterDocument( TextDocument* doc ) override;

	virtual void onUnregisterDocument( TextDocument* doc ) override;

	bool isOverMatch( TextDocument* doc, const Int64& index ) const;

	bool isVisibleInRange( TextDocument* doc, const DocumentLineRange& docLineRange );
};

} // namespace ecode

#endif // ECODE_XMLTOOLSPLUGIN_HPP
