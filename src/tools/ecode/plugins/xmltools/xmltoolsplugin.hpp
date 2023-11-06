#ifndef ECODE_XMLTOOLSPLUGIN_HPP
#define ECODE_XMLTOOLSPLUGIN_HPP

#include "../pluginmanager.hpp"
#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <set>
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
				 { 0, 0, 2 },
				 XMLToolsPlugin::NewSync };
	}

	static UICodeEditorPlugin* New( PluginManager* pluginManager );

	static UICodeEditorPlugin* NewSync( PluginManager* pluginManager );

	virtual ~XMLToolsPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	bool getHighlightMatch() const;

	bool getAutoEditMatch() const;

	void drawBeforeLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							 const Float& fontSize, const Float& lineHeight ) override;

	void minimapDrawAfterLineText( UICodeEditor*, const Int64&, const Vector2f&, const Vector2f&,
								   const Float&, const Float& ) override;

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
		virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& ){};
		virtual void onDocumentCursorChange( const TextPosition& ){};
		virtual void onDocumentInterestingCursorChange( const TextPosition& ){};
		virtual void onDocumentSelectionChange( const TextRange& );
		virtual void onDocumentLineCountChange( const size_t&, const size_t& ){};
		virtual void onDocumentLineChanged( const Int64& ){};
		virtual void onDocumentSaved( TextDocument* ){};
		virtual void onDocumentClosed( TextDocument* ){};
		virtual void onDocumentDirtyOnFileSystem( TextDocument* ){};
		virtual void onDocumentMoved( TextDocument* ){};

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
};

} // namespace ecode

#endif // ECODE_XMLTOOLSPLUGIN_HPP
