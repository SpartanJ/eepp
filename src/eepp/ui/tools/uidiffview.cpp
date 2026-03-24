#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/thirdparty/dtl/dtl.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/tools/uidiffview.hpp>

namespace EE { namespace UI { namespace Tools {

class UIDiffEditorPlugin : public UICodeEditorPlugin {
  public:
	UIDiffEditorPlugin( UIDiffView* view ) : mView( view ) {}

	inline Color getBackgroundColor( UIDiffView::DiffLineType type ) {
		switch( type ) {
			case UIDiffView::DiffLineType::Added:
				return Color( 0, 150, 32, 30 );
			case UIDiffView::DiffLineType::Removed:
				return Color( 180, 0, 32, 30 );
			case UIDiffView::DiffLineType::Header:
				return Color( 100, 100, 100, 30 );
			case UIDiffView::DiffLineType::Common:
				break;
		}
		return Color::Transparent;
	}

	std::string getId() override { return "UIDiffEditorPlugin"; }
	std::string getTitle() override { return "UIDiffEditorPlugin"; }
	std::string getDescription() override { return "Highlights diff added/removed lines."; }
	bool isReady() const override { return true; }

	void onRegister( UICodeEditor* editor ) override {
		editor->registerGutterSpace( this, PixelDensity::dpToPx( 80 ), 0 );
	}

	void onUnregister( UICodeEditor* editor ) override { editor->unregisterGutterSpace( this ); }

	void drawBeforeLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							 const Float& /*fontSize*/, const Float& lineHeight ) override {
		if ( !mView || index < 0 || index >= (Int64)mView->getDiffLines().size() )
			return;

		const auto& lines = mView->getDiffLines();
		const auto& line = lines[index];
		Color bgColor( getBackgroundColor( line.type ) );

		if ( bgColor != Color::Transparent ) {
			Primitives p;
			p.setColor( bgColor );
			p.drawRectangle( Rectf( Vector2f( editor->getScreenPos().x, position.y ),
									Sizef( editor->getPixelsSize().getWidth(), lineHeight ) ) );
		}
	}

	void drawGutter( UICodeEditor* editor, const Int64& index, const Vector2f& screenStart,
					 const Float& lineHeight, const Float& gutterWidth,
					 const Float& fontSize ) override {
		if ( !mView || index < 0 || index >= (Int64)mView->getDiffLines().size() )
			return;

		const auto& lines = mView->getDiffLines();
		const auto& line = lines[index];

		String text;
		if ( line.type == UIDiffView::DiffLineType::Added ) {
			text = String::format( "%5s %5lld", "", (long long)line.newLineNum );
		} else if ( line.type == UIDiffView::DiffLineType::Removed ) {
			text = String::format( "%5lld %5s", (long long)line.oldLineNum, "" );
		} else if ( line.type == UIDiffView::DiffLineType::Common ) {
			text = String::format( "%5lld %5lld", (long long)line.oldLineNum,
								   (long long)line.newLineNum );
		} else {
			return;
		}

		FontStyleConfig config = editor->getFontStyleConfig();
		config.FontColor = editor->getColorScheme().getEditorColor( SyntaxStyleTypes::LineNumber );

		Float textWidth = Text::getTextWidth( text, config, 4, TextHints::AllAscii );

		Vector2f pos( screenStart.x + ( gutterWidth - textWidth ) * 0.5f,
					  screenStart.y +
						  ( lineHeight - config.Font->getLineSpacing( config.CharacterSize ) ) *
							  0.5f );

		Text::draw( text, pos, config, 4, TextHints::AllAscii );
	}

  protected:
	UIDiffView* mView;
};

UIDiffView* UIDiffView::New() {
	return eeNew( UIDiffView, () );
}

UIDiffView::UIDiffView() : UIWidget( "diffview" ) {
	setFlags( UI_AUTO_SIZE );
	mEditor = UICodeEditor::New();
	mEditor->setParent( this );
	mEditor->setDocument( std::make_shared<Doc::TextDocument>() );
	mEditor->setLocked( true );
	mEditor->setShowLineNumber( false );
	mEditor->setShowFoldingRegion( false );

	mPlugin = std::make_shared<UIDiffEditorPlugin>( this );
	mEditor->registerPlugin( mPlugin.get() );
}

UIDiffView::~UIDiffView() {
	if ( mEditor && mPlugin ) {
		mEditor->unregisterPlugin( mPlugin.get() );
	}
}

Uint32 UIDiffView::getType() const {
	return UI_TYPE_DIFF_VIEW;
}

bool UIDiffView::isType( const Uint32& type ) const {
	return UIDiffView::getType() == type ? true : UIWidget::isType( type );
}

void UIDiffView::onSizeChange() {
	if ( mEditor )
		mEditor->setPixelsSize( getPixelsSize() );
	UIWidget::onSizeChange();
}

void UIDiffView::loadFromPatch( const std::string& patchText ) {
	mLines.clear();
	auto lines = String::split( patchText, '\n' );
	std::string cleanText;

	Int64 oldLineNum = 0;
	Int64 newLineNum = 0;
	std::string filename;

	for ( const auto& line : lines ) {
		if ( String::startsWith( line, "diff " ) || String::startsWith( line, "index " ) ||
			 String::startsWith( line, "--- " ) || String::startsWith( line, "+++ " ) ) {
			if ( String::startsWith( line, "+++ " ) ) {
				filename = line.substr( 4 );
				if ( String::startsWith( filename, "b/" ) )
					filename = filename.substr( 2 );
				filename = String::trim( filename );
			}
			continue;
		}

		if ( String::startsWith( line, "@@ " ) ) {
			size_t minusPos = line.find( "-" );
			size_t plusPos = line.find( "+" );
			if ( minusPos != std::string::npos && plusPos != std::string::npos ) {
				size_t commaPos = line.find( ",", minusPos );
				size_t spacePos = line.find( " ", minusPos );
				if ( commaPos != std::string::npos && commaPos < spacePos ) {
					oldLineNum =
						std::stoll( line.substr( minusPos + 1, commaPos - minusPos - 1 ) ) - 1;
				} else if ( spacePos != std::string::npos ) {
					oldLineNum =
						std::stoll( line.substr( minusPos + 1, spacePos - minusPos - 1 ) ) - 1;
				}

				commaPos = line.find( ",", plusPos );
				spacePos = line.find( " ", plusPos );
				if ( commaPos != std::string::npos && commaPos < spacePos ) {
					newLineNum =
						std::stoll( line.substr( plusPos + 1, commaPos - plusPos - 1 ) ) - 1;
				} else if ( spacePos != std::string::npos ) {
					newLineNum =
						std::stoll( line.substr( plusPos + 1, spacePos - plusPos - 1 ) ) - 1;
				}
			}
			continue;
		}

		DiffLine dline;
		dline.text = line;

		if ( String::startsWith( line, "+" ) ) {
			dline.type = DiffLineType::Added;
			dline.text = line.substr( 1 );
			dline.newLineNum = ++newLineNum;
		} else if ( String::startsWith( line, "-" ) ) {
			dline.type = DiffLineType::Removed;
			dline.text = line.substr( 1 );
			dline.oldLineNum = ++oldLineNum;
		} else if ( String::startsWith( line, " " ) ) {
			dline.type = DiffLineType::Common;
			dline.text = line.substr( 1 );
			dline.oldLineNum = ++oldLineNum;
			dline.newLineNum = ++newLineNum;
		} else {
			dline.type = DiffLineType::Common;
			dline.oldLineNum = ++oldLineNum;
			dline.newLineNum = ++newLineNum;
		}

		cleanText += dline.text + "\n";
		mLines.push_back( dline );
	}

	mEditor->getDocument().reset();
	mEditor->getDocument().textInput( cleanText );

	if ( !filename.empty() ) {
		auto def = Doc::SyntaxDefinitionManager::instance()->getByExtension( filename );
		if ( def.getLanguageIndex() > 1 )
			mEditor->getDocument().setSyntaxDefinition( def );
	}
}

void UIDiffView::loadFromStrings( const std::string& oldText, const std::string& newText ) {
	mLines.clear();

	std::vector<std::string> leftLines = String::split( oldText, '\n' );
	std::vector<std::string> rightLines = String::split( newText, '\n' );

	dtl::Diff<std::string> diff( leftLines, rightLines );
	diff.compose();
	auto ranges = diff.getSes().getSequence();

	size_t leftIndex = 0;
	size_t rightIndex = 0;
	std::string cleanText;

	for ( auto& pair : ranges ) {
		DiffLine dline;
		dline.text = pair.first;
		switch ( pair.second.type ) {
			case dtl::SES_COMMON:
				dline.type = DiffLineType::Common;
				dline.oldLineNum = ++leftIndex;
				dline.newLineNum = ++rightIndex;
				break;
			case dtl::SES_ADD:
				dline.type = DiffLineType::Added;
				dline.newLineNum = ++rightIndex;
				break;
			case dtl::SES_DELETE:
				dline.type = DiffLineType::Removed;
				dline.oldLineNum = ++leftIndex;
				break;
		}
		mLines.push_back( dline );
		cleanText += dline.text + "\n";
	}

	mEditor->getDocument().reset();
	mEditor->getDocument().textInput( cleanText );
}

void UIDiffView::loadFromFile( const std::string& oldFilePath, const std::string& newFilePath ) {
	std::string oldText, newText;
	FileSystem::fileGet( oldFilePath, oldText );
	FileSystem::fileGet( newFilePath, newText );
	loadFromStrings( oldText, newText );

	auto def = Doc::SyntaxDefinitionManager::instance()->getByExtension( oldFilePath );
	if ( def.getLanguageIndex() > 1 )
		mEditor->getDocument().setSyntaxDefinition( def );
}

}}} // namespace EE::UI::Tools
