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
		Float glyphWidth = editor->getGlyphWidth();
		Float totalChars = mView->getViewMode() == UIDiffView::ViewMode::Unified ? 10 : 5;
		mGutterWidth = PixelDensity::dpToPx( glyphWidth * totalChars );
		editor->registerGutterSpace( this, mGutterWidth, 0 );
	}

	void onUnregister( UICodeEditor* editor ) override { editor->unregisterGutterSpace( this ); }

	void registerUpdate( UICodeEditor* editor ) {
		onUnregister( editor );
		onRegister( editor );
	}

	void drawBeforeLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							 const Float& /*fontSize*/, const Float& lineHeight ) override {
		const auto& viewLines = mView->getViewLines();
		if ( !mView || index < 0 || index >= (Int64)viewLines.size() )
			return;

		const auto& lines = mView->getDiffLines();
		const auto& line = lines[viewLines[index]];
		Color bgColor( getBackgroundColor( line.type ) );

		if ( bgColor == Color::Transparent )
			return;

		Primitives p;
		p.setColor( bgColor );
		p.drawRectangle(
			Rectf( Vector2f( editor->getScreenPos().x + mGutterWidth, position.y ),
				   Sizef( editor->getPixelsSize().getWidth() - mGutterWidth, lineHeight ) ) );

		if ( line.subLineChanges.empty() )
			return;

		p.setColor( getBackgroundColor( line.type, true ) );

		for ( const auto& range : line.subLineChanges ) {
			Vector2f startPos =
				editor->getTextPositionOffset( { index, range.start().column() }, lineHeight )
					.asFloat();

			Vector2f endPos =
				editor->getTextPositionOffset( { index, range.end().column() }, lineHeight )
					.asFloat();

			p.drawRectangle( Rectf( { position.x + startPos.x, position.y },
									{ endPos.x - startPos.x, lineHeight } ) );
		}
	}

	void drawAfterLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							const Float& /*fontSize*/, const Float& lineHeight ) override {
		const auto& viewLines = mView->getViewLines();
		if ( !mView || index < 0 || index >= (Int64)viewLines.size() )
			return;
		const auto& lines = mView->getDiffLines();
		const auto& line = lines[viewLines[index]];
		Primitives p;
		Vector2f startScroll = Vector2f{ editor->getScreenStart().x, position.y };
		Rectf rect( startScroll, Sizef( mGutterWidth, lineHeight ) );
		Color bgColor( editor->getLineNumberBackgroundColor() );
		if ( editor->getScroll().x != 0 ) {
			bgColor.blendAlpha( editor->getAlpha() );
			p.setColor( bgColor );
			p.drawRectangle( rect );
		}
		bgColor = getBackgroundColor( line.type );
		if ( bgColor == Color::Transparent )
			return;
		p.setColor( bgColor );
		p.drawRectangle( rect );
	}

	void drawGutter( UICodeEditor* editor, const Int64& index, const Vector2f& screenStart,
					 const Float& lineHeight, const Float& gutterWidth,
					 const Float& /*fontSize*/ ) override {
		const auto& viewLines = mView->getViewLines();
		if ( !mView || index < 0 || index >= (Int64)viewLines.size() )
			return;

		const auto& lines = mView->getDiffLines();
		const auto& line = lines[viewLines[index]];

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
					String::formatBuffer( buf, bufSize, "%5lld %5s", (long long)line.oldLineNum,
										  "" );
					break;
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

		String::View text{ buf };
		if ( text.empty() )
			return;

		FontStyleConfig config = editor->getFontStyleConfig();
		config.FontColor = editor->getColorScheme().getEditorColor( SyntaxStyleTypes::LineNumber );

		Float textWidth = Text::getTextWidth( text, config, 4, TextHints::AllAscii );

		Vector2f pos( screenStart.x + std::floor( ( mGutterWidth - textWidth ) * 0.5f ),
					  screenStart.y + std::floor( ( lineHeight - config.Font->getLineSpacing(
																	 config.CharacterSize ) ) *
												  0.5f ) );

		Text::draw( text, pos, config, 4, TextHints::AllAscii );
	}

  protected:
	UIDiffView* mView;
	Float mGutterWidth{ 0 };
};

UIDiffView* UIDiffView::New() {
	return eeNew( UIDiffView, () );
}

UIDiffView::UIDiffView() : UIWidget( "diffview" ) {
	setFlags( UI_AUTO_SIZE );
	createEditor( mEditor, mPlugin );
	createEditor( mLeftEditor, mLeftPlugin );
	createEditor( mRightEditor, mRightPlugin );

	mEditor->on( Event::OnFontChanged, [this]( auto ) { mPlugin->registerUpdate( mEditor ); } );
	mLeftEditor->on( Event::OnFontChanged, [this]( auto ) {
		mLeftPlugin->registerUpdate( mLeftEditor );
		mRightEditor->setFontSize( mLeftEditor->getFontSize() );
		mRightPlugin->registerUpdate( mRightEditor );
	} );
	mRightEditor->on( Event::OnFontChanged, [this]( auto ) {
		mRightPlugin->registerUpdate( mRightEditor );
		mLeftEditor->setFontSize( mRightEditor->getFontSize() );
		mLeftPlugin->registerUpdate( mLeftEditor );
	} );

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
	mModeToggle->onClick( [this]( const Event* ) {
		setViewMode( mViewMode == ViewMode::Unified ? ViewMode::SideBySide : ViewMode::Unified );
	} );

	mModeToggle->on( Event::OnSizeChange, [this]( auto ) { updateModeButton(); } );

	mCompleteViewToggle = UIPushButton::New();
	mCompleteViewToggle->setParent( this );
	mCompleteViewToggle->onClick(
		[this]( const Event* ) { setCompleteView( !mShowCompleteView ); } );

	mCompleteViewToggle->on( Event::OnSizeChange, [this]( auto ) { updateModeButton(); } );

	updateButtonsText();
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

	mLeftPlugin->registerUpdate( mLeftEditor );
	mRightPlugin->registerUpdate( mRightEditor );

	if ( mViewMode == ViewMode::Unified ) {
		mEditor->setVisible( true );
		mLeftEditor->setVisible( false );
		mRightEditor->setVisible( false );
		onSizeChange();
		syncScroll( mRightEditor, mEditor, true );
	} else {
		mEditor->setVisible( false );
		mLeftEditor->setVisible( true );
		mRightEditor->setVisible( true );
		onSizeChange();
		syncScroll( mEditor, mRightEditor, true );
		syncScroll( mEditor, mLeftEditor, true );
	}

	updateButtonsText();
}

void UIDiffView::setViewModeToggleVisible( bool visible ) {
	mViewModeToggleVisible = visible;
	mModeToggle->setVisible( visible );
	updateModeButton();
}

void UIDiffView::setCompleteView( bool complete ) {
	if ( mShowCompleteView == complete )
		return;
	mShowCompleteView = complete;
	updateButtonsText();
	updateEditorsText();
}

void UIDiffView::setCompleteViewToggleVisible( bool visible ) {
	mCompleteViewToggleVisible = visible;
	mCompleteViewToggle->setVisible( visible );
	updateModeButton();
}

void UIDiffView::setSubLineDiffAlgorithm( SubLineDiffAlgorithm algo ) {
	mSubLineDiffAlgorithm = algo;
}

void UIDiffView::syncScroll( UICodeEditor* source, UICodeEditor* target, bool emitEvent ) {
	target->setScrollY( source->getScroll().y, emitEvent );
	target->setScrollX( source->getScroll().x, emitEvent );
}

void UIDiffView::updateModeButton() {
	auto margin = PixelDensity::dpToPx( 8 );

	Float currentX = getPixelsSize().getWidth() - margin;
	currentX -= mRightEditor->getVScrollBar()->getPixelsSize().getWidth();

	if ( mViewModeToggleVisible && mModeToggle ) {
		currentX -= mModeToggle->getPixelsSize().getWidth();
		mModeToggle->setPixelsPosition( currentX, margin );
		currentX -= margin;
	}

	if ( mCompleteViewToggleVisible && mCompleteViewToggle ) {
		currentX -= mCompleteViewToggle->getPixelsSize().getWidth();
		mCompleteViewToggle->setPixelsPosition( currentX, margin );
	}
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

void UIDiffView::updateEditorsText() {
	String cleanText;
	String leftText;
	String rightText;
	mViewLines.clear();

	for ( size_t i = 0; i < mLines.size(); ++i ) {
		bool showLine = mShowCompleteView;

		if ( !showLine ) {
			int ctx = 3;
			int start = std::max( 0, (int)i - ctx );
			int end = std::min( (int)mLines.size() - 1, (int)i + ctx );
			for ( int j = start; j <= end; ++j ) {
				if ( mLines[j].type != DiffLineType::Common &&
					 mLines[j].type != DiffLineType::Header ) {
					showLine = true;
					break;
				}
			}
		}

		if ( showLine ) {
			mViewLines.push_back( i );
			if ( mLines[i].type == DiffLineType::Added ) {
				leftText += "\n";
				rightText += mLines[i].text + "\n";
			} else if ( mLines[i].type == DiffLineType::Removed ) {
				leftText += mLines[i].text + "\n";
				rightText += "\n";
			} else {
				leftText += mLines[i].text + "\n";
				rightText += mLines[i].text + "\n";
			}
			cleanText += mLines[i].text + "\n";
		}
	}

	mEditor->getDocument().reset();
	mEditor->getDocument().textInput( cleanText );
	mLeftEditor->getDocument().reset();
	mLeftEditor->getDocument().textInput( leftText );
	mRightEditor->getDocument().reset();
	mRightEditor->getDocument().textInput( rightText );
	mEditor->getDocument().setSelection( TextPosition( 0, 0 ) );
	mLeftEditor->getDocument().setSelection( TextPosition( 0, 0 ) );
	mRightEditor->getDocument().setSelection( TextPosition( 0, 0 ) );

	if ( mSyntaxDef ) {
		mEditor->getDocument().setSyntaxDefinition( mSyntaxDef );
		mLeftEditor->getDocument().setSyntaxDefinition( mSyntaxDef );
		mRightEditor->getDocument().setSyntaxDefinition( mSyntaxDef );
	}
}

void UIDiffView::computeSubLineDiff( DiffLine& oldLine, DiffLine& newLine ) {
	dtl::Diff<String::StringBaseType, String::View> diff( oldLine.text.view(),
														  newLine.text.view() );
	diff.compose();

	if ( mSubLineDiffAlgorithm == SubLineDiffAlgorithm::SES ) {
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
	} else {
		auto lcs = diff.getLcsVec();
		Int64 oldIdx = 0;
		Int64 newIdx = 0;
		Int64 lcsIdx = 0;

		while ( oldIdx < (Int64)oldLine.text.size() || newIdx < (Int64)newLine.text.size() ) {
			if ( lcsIdx < (Int64)lcs.size() && oldIdx < (Int64)oldLine.text.size() &&
				 newIdx < (Int64)newLine.text.size() && oldLine.text[oldIdx] == lcs[lcsIdx] &&
				 newLine.text[newIdx] == lcs[lcsIdx] ) {
				oldIdx++;
				newIdx++;
				lcsIdx++;
			} else {
				if ( oldIdx < (Int64)oldLine.text.size() &&
					 ( lcsIdx == (Int64)lcs.size() || oldLine.text[oldIdx] != lcs[lcsIdx] ) ) {
					if ( !oldLine.subLineChanges.empty() &&
						 oldLine.subLineChanges.back().end().column() == oldIdx ) {
						oldLine.subLineChanges.back().setEnd( { 0, oldIdx + 1 } );
					} else {
						oldLine.subLineChanges.push_back( { { 0, oldIdx }, { 0, oldIdx + 1 } } );
					}
					oldIdx++;
				}
				if ( newIdx < (Int64)newLine.text.size() &&
					 ( lcsIdx == (Int64)lcs.size() || newLine.text[newIdx] != lcs[lcsIdx] ) ) {
					if ( !newLine.subLineChanges.empty() &&
						 newLine.subLineChanges.back().end().column() == newIdx ) {
						newLine.subLineChanges.back().setEnd( { 0, newIdx + 1 } );
					} else {
						newLine.subLineChanges.push_back( { { 0, newIdx }, { 0, newIdx + 1 } } );
					}
					newIdx++;
				}
			}
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

void UIDiffView::loadFromPatch( const std::string& patchText,
								const std::string& originalFilePath ) {
	mLines.clear();
	auto lines = String::split( patchText, '\n', true );

	std::string fileText;
	bool hasCompleteFile = false;
	std::vector<std::string> fileLines;
	if ( !originalFilePath.empty() && FileSystem::fileExists( originalFilePath ) ) {
		FileSystem::fileGet( originalFilePath, fileText );
		fileLines = String::split( fileText, '\n', true );
		hasCompleteFile = true;
	}

	Int64 oldLineNum = 0;
	Int64 newLineNum = 0;
	Int64 expectedOldLineNum = 1;
	Int64 expectedNewLineNum = 1;
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

				if ( hasCompleteFile ) {
					while ( expectedNewLineNum < newLineNum + 1 &&
							expectedNewLineNum <= (Int64)fileLines.size() ) {
						DiffLine dline;
						dline.type = DiffLineType::Common;
						dline.text = fileLines[expectedNewLineNum - 1];
						dline.oldLineNum = expectedOldLineNum++;
						dline.newLineNum = expectedNewLineNum++;
						mLines.push_back( dline );
					}
				}
				expectedOldLineNum = oldLineNum + 1;
				expectedNewLineNum = newLineNum + 1;
			}
			continue;
		}

		DiffLine dline;
		dline.text = line;

		if ( String::startsWith( line, "+" ) ) {
			dline.type = DiffLineType::Added;
			dline.text = line.substr( 1 );
			dline.newLineNum = ++newLineNum;
			expectedNewLineNum = newLineNum + 1;
		} else if ( String::startsWith( line, "-" ) ) {
			dline.type = DiffLineType::Removed;
			dline.text = line.substr( 1 );
			dline.oldLineNum = ++oldLineNum;
			expectedOldLineNum = oldLineNum + 1;
		} else if ( String::startsWith( line, " " ) ) {
			dline.type = DiffLineType::Common;
			dline.text = line.substr( 1 );
			dline.oldLineNum = ++oldLineNum;
			dline.newLineNum = ++newLineNum;
			expectedOldLineNum = oldLineNum + 1;
			expectedNewLineNum = newLineNum + 1;
		} else {
			dline.type = DiffLineType::Common;
			dline.oldLineNum = ++oldLineNum;
			dline.newLineNum = ++newLineNum;
			expectedOldLineNum = oldLineNum + 1;
			expectedNewLineNum = newLineNum + 1;
		}

		mLines.push_back( dline );
	}

	if ( hasCompleteFile ) {
		while ( expectedNewLineNum <= (Int64)fileLines.size() ) {
			DiffLine dline;
			dline.type = DiffLineType::Common;
			dline.text = fileLines[expectedNewLineNum - 1];
			dline.oldLineNum = expectedOldLineNum++;
			dline.newLineNum = expectedNewLineNum++;
			mLines.push_back( dline );
		}
	}

	applySubLineDiff( mLines, [this]( DiffLine& oldLine, DiffLine& newLine ) {
		computeSubLineDiff( oldLine, newLine );
	} );

	setCompleteViewToggleVisible( hasCompleteFile );

	if ( !filename.empty() ) {
		auto def = SyntaxDefinitionManager::instance()->getByExtension( filename );
		mSyntaxDef =
			SyntaxDefinitionManager::instance()->getLanguageDefinition( def.getLanguageIndex() );
	}

	updateEditorsText();
	updateButtonsText();
}

void UIDiffView::loadFromStrings( const std::string& oldText, const std::string& newText ) {
	mLines.clear();

	std::vector<std::string> leftLines = String::split( oldText, '\n', true );
	std::vector<std::string> rightLines = String::split( newText, '\n', true );

	dtl::Diff<std::string> diff( leftLines, rightLines );
	diff.compose();
	auto ranges = diff.getSes().getSequence();

	size_t leftIndex = 0;
	size_t rightIndex = 0;

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
	}

	applySubLineDiff( mLines, [this]( DiffLine& oldLine, DiffLine& newLine ) {
		computeSubLineDiff( oldLine, newLine );
	} );

	mShowCompleteView = false;
	mCompleteViewToggle->setText( i18n( "diffview_complete", "Complete" ) );
	updateEditorsText();
}

void UIDiffView::loadFromFile( const std::string& oldFilePath, const std::string& newFilePath ) {
	std::string oldText, newText;
	FileSystem::fileGet( oldFilePath, oldText );
	FileSystem::fileGet( newFilePath, newText );

	auto def = SyntaxDefinitionManager::instance()->getByExtension( oldFilePath );
	mSyntaxDef =
		SyntaxDefinitionManager::instance()->getLanguageDefinition( def.getLanguageIndex() );
	mEditor->getDocument().setSyntaxDefinition( def );
	mLeftEditor->getDocument().setSyntaxDefinition( def );
	mRightEditor->getDocument().setSyntaxDefinition( def );

	loadFromStrings( oldText, newText );
}

void UIDiffView::updateButtonsText() {
	mModeToggle->setText( mViewMode == ViewMode::Unified ? i18n( "diffview_split", "Split" )
														 : i18n( "diffview_unified", "Unified" ) );

	mCompleteViewToggle->setText( mShowCompleteView ? i18n( "diffview_reduced", "Reduced" )
													: i18n( "diffview_complete", "Complete" ) );
}

}}} // namespace EE::UI::Tools
