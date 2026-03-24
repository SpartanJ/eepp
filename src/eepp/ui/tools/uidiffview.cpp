#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>

#include <dtl/dtl.hpp>

namespace EE { namespace UI { namespace Tools {

class UIDiffEditorPlugin : public UICodeEditorPlugin {
  public:
	UIDiffEditorPlugin( UIDiffView* view ) : mView( view ) {}

	inline Color getBackgroundColor( UIDiffView::DiffLineType type, bool isHighlight = false ) {
		static constexpr auto NORMAL_ALPHA = 40;
		static constexpr auto HIGHLIGHT_ALPHA = 80;
		switch ( type ) {
			case UIDiffView::DiffLineType::Added:
				return Color( 0, 150, 32, isHighlight ? HIGHLIGHT_ALPHA : NORMAL_ALPHA );
			case UIDiffView::DiffLineType::Removed:
				return Color( 180, 0, 32, isHighlight ? HIGHLIGHT_ALPHA : NORMAL_ALPHA );
			case UIDiffView::DiffLineType::Header:
				return Color( 100, 100, 100, isHighlight ? HIGHLIGHT_ALPHA : NORMAL_ALPHA );
			case UIDiffView::DiffLineType::Common:
				break;
		}
		return Color::Transparent;
	}

	std::string getId() override { return "DiffEditorPlugin"; }

	std::string getTitle() override { return "DiffEditorPlugin"; }

	std::string getDescription() override { return "Highlights diff added/removed lines."; }

	bool isReady() const override { return true; }

	void onRegister( UICodeEditor* editor ) override {
		if ( mView->getViewMode() == UIDiffView::ViewMode::Unified ) {
			editor->registerGutterSpace( this, PixelDensity::dpToPx( 80 ), 0 );
		} else {
			editor->registerGutterSpace( this, PixelDensity::dpToPx( 40 ), 0 );
		}
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

			if ( !line.subLineChanges.empty() ) {
				p.setColor( getBackgroundColor( line.type, true ) );

				for ( const auto& range : line.subLineChanges ) {
					Vector2f startPos =
						editor
							->getTextPositionOffset( { index, range.start().column() }, lineHeight )
							.asFloat();

					Vector2f endPos =
						editor->getTextPositionOffset( { index, range.end().column() }, lineHeight )
							.asFloat();

					p.drawRectangle( Rectf( { position.x + startPos.x, position.y },
											{ endPos.x - startPos.x, lineHeight } ) );
				}
			}
		}
	}

	void drawGutter( UICodeEditor* editor, const Int64& index, const Vector2f& screenStart,
					 const Float& lineHeight, const Float& gutterWidth,
					 const Float& fontSize ) override {
		if ( !mView || index < 0 || index >= (Int64)mView->getDiffLines().size() )
			return;

		const auto& lines = mView->getDiffLines();
		const auto& line = lines[index];

		static constexpr auto bufSize = 16;
		String::StringBaseType buf[bufSize] = {};
		if ( mView->getViewMode() == UIDiffView::ViewMode::Unified ) {
			switch ( line.type ) {
				case UIDiffView::DiffLineType::Added:
					String::formatBuffer( buf, bufSize, "%5s %5lld", "",
										  (long long)line.newLineNum );
					break;
				case UIDiffView::DiffLineType::Removed:
					String::formatBuffer( buf, bufSize, "%5lld %5s", (long long)line.oldLineNum,
										  "" );
					break;
				case UIDiffView::DiffLineType::Header:
					String::formatBuffer( buf, bufSize, "%5lld %5lld", (long long)line.oldLineNum,
										  (long long)line.newLineNum );
					break;
				case UIDiffView::DiffLineType::Common:
					return;
			}
		} else {
			if ( editor == mView->getLeftEditor() ) {
				if ( line.oldLineNum > 0 )
					String::formatBuffer( buf, bufSize, "%5lld", (long long)line.oldLineNum );
			} else {
				if ( line.newLineNum > 0 )
					String::formatBuffer( buf, bufSize, "%5lld", (long long)line.newLineNum );
			}
		}

		String::View text{ buf, bufSize };
		if ( text.empty() )
			return;

		FontStyleConfig config = editor->getFontStyleConfig();
		config.FontColor = editor->getColorScheme().getEditorColor( SyntaxStyleTypes::LineNumber );

		Float textWidth = Text::getTextWidth( text, config, 4, TextHints::AllAscii );

		Vector2f pos( screenStart.x + std::floor( ( gutterWidth - textWidth ) * 0.5f ),
					  screenStart.y + std::floor( ( lineHeight - config.Font->getLineSpacing(
																	 config.CharacterSize ) ) *
												  0.5f ) );

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
	createEditor( mEditor, mPlugin );
	createEditor( mLeftEditor, mLeftPlugin );
	createEditor( mRightEditor, mRightPlugin );

	mLeftEditor->setVisible( false );
	mRightEditor->setVisible( false );
	mLeftEditor->setVerticalScrollBarEnabled( false );

	mLeftEditor->on( Event::OnScrollChange, [this]( const Event* ) {
		syncScroll( mLeftEditor, mRightEditor );

		mRightEditor->getVScrollBar()->setValue(
			mLeftEditor->getScroll().y / mLeftEditor->getMaxScroll().y, false );
	} );

	mRightEditor->on( Event::OnScrollChange,
					  [this]( const Event* ) { syncScroll( mRightEditor, mLeftEditor ); } );

	mLeftEditor->getVScrollBar()->on(
		Event::OnValueChange, [this]( const Event* ) { syncScroll( mLeftEditor, mRightEditor ); } );

	mRightEditor->getVScrollBar()->on(
		Event::OnValueChange, [this]( const Event* ) { syncScroll( mRightEditor, mLeftEditor ); } );

	mModeToggle = UIPushButton::New();
	mModeToggle->setParent( this );
	mModeToggle->setText( "Unified" );
	mModeToggle->onClick( [this]( const Event* event ) {
		setViewMode( mViewMode == ViewMode::Unified ? ViewMode::SideBySide : ViewMode::Unified );
	} );

	mModeToggle->on( Event::OnSizeChange, [this]( auto ) { updateModeButton(); } );
}

UIDiffView::~UIDiffView() {
	if ( mEditor && mPlugin )
		mEditor->unregisterPlugin( mPlugin.get() );

	if ( mLeftEditor && mLeftPlugin )
		mLeftEditor->unregisterPlugin( mLeftPlugin.get() );

	if ( mRightEditor && mRightPlugin )
		mRightEditor->unregisterPlugin( mRightPlugin.get() );
}

Uint32 UIDiffView::getType() const {
	return UI_TYPE_DIFF_VIEW;
}

bool UIDiffView::isType( const Uint32& type ) const {
	return UIDiffView::getType() == type ? true : UIWidget::isType( type );
}

void UIDiffView::createEditor( UICodeEditor*& editor,
							   std::unique_ptr<UIDiffEditorPlugin>& plugin ) {
	editor = UICodeEditor::New();
	editor->setParent( this );
	editor->setLocked( true );
	editor->setShowLineNumber( false );
	editor->setShowFoldingRegion( false );
	editor->setDisableScrollInvalidation( true );
	plugin = std::make_unique<UIDiffEditorPlugin>( this );
	editor->registerPlugin( plugin.get() );
}

void UIDiffView::setViewMode( ViewMode mode ) {
	if ( mViewMode == mode )
		return;

	mViewMode = mode;

	if ( mViewMode == ViewMode::Unified ) {
		mEditor->setVisible( true );
		mLeftEditor->setVisible( false );
		mRightEditor->setVisible( false );
		mModeToggle->setText( i18n( "diffview_unified", "Unified" ) );
		onSizeChange();
		syncScroll( mRightEditor, mEditor, true );
	} else {
		mEditor->setVisible( false );
		mLeftEditor->setVisible( true );
		mRightEditor->setVisible( true );
		mModeToggle->setText( i18n( "diffview_split", "Split" ) );
		onSizeChange();
		syncScroll( mEditor, mRightEditor, true );
		syncScroll( mEditor, mLeftEditor, true );
	}
}

void UIDiffView::setViewModeToggleVisible( bool visible ) {
	mViewModeToggleVisible = visible;
	mModeToggle->setVisible( visible );
}

void UIDiffView::syncScroll( UICodeEditor* source, UICodeEditor* target, bool emitEvent ) {
	target->setScrollY( source->getScroll().y, emitEvent );
	target->setScrollX( source->getScroll().x, emitEvent );
}

void UIDiffView::updateModeButton() {
	mModeToggle->setPixelsPosition(
		getPixelsSize().getWidth() - mModeToggle->getPixelsSize().getWidth() -
			mRightEditor->getVScrollBar()->getPixelsSize().getWidth() - PixelDensity::dpToPx( 8 ),
		PixelDensity::dpToPx( 8 ) );
}

void UIDiffView::onSizeChange() {
	if ( mViewMode == ViewMode::Unified ) {
		mEditor->setPixelsSize( getPixelsSize() );
	} else {
		mLeftEditor->setPixelsSize( std::ceil( getPixelsSize().getWidth() * 0.5f ),
									getPixelsSize().getHeight() );
		mRightEditor->setPixelsSize( std::floor( getPixelsSize().getWidth() * 0.5f ),
									 getPixelsSize().getHeight() );
		mRightEditor->setPixelsPosition( std::floor( getPixelsSize().getWidth() * 0.5f ), 0.f );
	}

	updateModeButton();
}

void UIDiffView::computeSubLineDiff( DiffLine& oldLine, DiffLine& newLine ) {
	dtl::Diff<String::StringBaseType, String::View> diff( oldLine.text.view(),
														  newLine.text.view() );
	diff.compose();
	auto ses = diff.getSes().getSequence();

	Int64 oldIdx = 0;
	Int64 newIdx = 0;
	for ( const auto& pair : ses ) {
		switch ( pair.second.type ) {
			case dtl::SES_COMMON:
				oldIdx++;
				newIdx++;
				break;
			case dtl::SES_ADD:
				if ( !newLine.subLineChanges.empty() &&
					 newLine.subLineChanges.back().end().column() == newIdx ) {
					newLine.subLineChanges.back().setEnd( { 0, newIdx + 1 } );
				} else {
					newLine.subLineChanges.push_back( { { 0, newIdx }, { 0, newIdx + 1 } } );
				}
				newIdx++;
				break;
			case dtl::SES_DELETE:
				if ( !oldLine.subLineChanges.empty() &&
					 oldLine.subLineChanges.back().end().column() == oldIdx ) {
					oldLine.subLineChanges.back().setEnd( { 0, oldIdx + 1 } );
				} else {
					oldLine.subLineChanges.push_back( { { 0, oldIdx }, { 0, oldIdx + 1 } } );
				}
				oldIdx++;
				break;
		}
	}
}

static void applySubLineDiff(
	std::vector<UIDiffView::DiffLine>& lines,
	std::function<void( UIDiffView::DiffLine&, UIDiffView::DiffLine& )> computeSubLineDiff ) {
	size_t i = 0;
	while ( i < lines.size() ) {
		if ( lines[i].type == UIDiffView::DiffLineType::Removed ) {
			size_t j = i;
			while ( j < lines.size() && lines[j].type == UIDiffView::DiffLineType::Removed )
				j++;
			size_t k = j;
			while ( k < lines.size() && lines[k].type == UIDiffView::DiffLineType::Added )
				k++;

			size_t numRemoved = j - i;
			size_t numAdded = k - j;
			size_t numToCompare = std::min( numRemoved, numAdded );
			for ( size_t m = 0; m < numToCompare; m++ ) {
				computeSubLineDiff( lines[i + m], lines[j + m] );
			}
			i = k;
		} else {
			i++;
		}
	}
}

void UIDiffView::loadFromPatch( const std::string& patchText ) {
	mLines.clear();
	auto lines = String::split( patchText, '\n' );
	String cleanText;
	String leftText;
	String rightText;

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
			leftText += "\n";
			rightText += dline.text + "\n";
		} else if ( String::startsWith( line, "-" ) ) {
			dline.type = DiffLineType::Removed;
			dline.text = line.substr( 1 );
			dline.oldLineNum = ++oldLineNum;
			leftText += dline.text + "\n";
			rightText += "\n";
		} else if ( String::startsWith( line, " " ) ) {
			dline.type = DiffLineType::Common;
			dline.text = line.substr( 1 );
			dline.oldLineNum = ++oldLineNum;
			dline.newLineNum = ++newLineNum;
			leftText += dline.text + "\n";
			rightText += dline.text + "\n";
		} else {
			dline.type = DiffLineType::Common;
			dline.oldLineNum = ++oldLineNum;
			dline.newLineNum = ++newLineNum;
			leftText += dline.text + "\n";
			rightText += dline.text + "\n";
		}

		cleanText += dline.text + "\n";
		mLines.push_back( dline );
	}

	applySubLineDiff( mLines, [this]( DiffLine& oldLine, DiffLine& newLine ) {
		computeSubLineDiff( oldLine, newLine );
	} );

	mEditor->getDocument().reset();
	mEditor->getDocument().textInput( cleanText );
	mLeftEditor->getDocument().reset();
	mLeftEditor->getDocument().textInput( leftText );
	mRightEditor->getDocument().reset();
	mRightEditor->getDocument().textInput( rightText );
	mEditor->getDocument().setSelection( TextPosition( 0, 0 ) );
	mLeftEditor->getDocument().setSelection( TextPosition( 0, 0 ) );
	mRightEditor->getDocument().setSelection( TextPosition( 0, 0 ) );

	if ( !filename.empty() ) {
		auto def = Doc::SyntaxDefinitionManager::instance()->getByExtension( filename );
		mEditor->getDocument().setSyntaxDefinition( def );
		mLeftEditor->getDocument().setSyntaxDefinition( def );
		mRightEditor->getDocument().setSyntaxDefinition( def );
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
	String cleanText;
	String leftText;
	String rightText;

	for ( auto& pair : ranges ) {
		DiffLine dline;
		dline.text = pair.first;
		switch ( pair.second.type ) {
			case dtl::SES_COMMON:
				dline.type = DiffLineType::Common;
				dline.oldLineNum = ++leftIndex;
				dline.newLineNum = ++rightIndex;
				leftText += dline.text + "\n";
				rightText += dline.text + "\n";
				break;
			case dtl::SES_ADD:
				dline.type = DiffLineType::Added;
				dline.newLineNum = ++rightIndex;
				leftText += "\n";
				rightText += dline.text + "\n";
				break;
			case dtl::SES_DELETE:
				dline.type = DiffLineType::Removed;
				dline.oldLineNum = ++leftIndex;
				leftText += dline.text + "\n";
				rightText += "\n";
				break;
		}
		cleanText += dline.text + "\n";
		mLines.push_back( dline );
	}

	applySubLineDiff( mLines, [this]( DiffLine& oldLine, DiffLine& newLine ) {
		computeSubLineDiff( oldLine, newLine );
	} );

	mEditor->getDocument().reset();
	mEditor->getDocument().textInput( cleanText );
	mLeftEditor->getDocument().reset();
	mLeftEditor->getDocument().textInput( leftText );
	mRightEditor->getDocument().reset();
	mRightEditor->getDocument().textInput( rightText );
	mEditor->getDocument().setSelection( TextPosition( 0, 0 ) );
	mLeftEditor->getDocument().setSelection( TextPosition( 0, 0 ) );
	mRightEditor->getDocument().setSelection( TextPosition( 0, 0 ) );
}

}}} // namespace EE::UI::Tools
