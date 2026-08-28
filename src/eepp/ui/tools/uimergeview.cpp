#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/window.hpp>

#include <dtl/dtl.hpp>

namespace EE::UI::Tools {

class UIMergeEditorPlugin : public UICodeEditorPlugin {
  public:
	explicit UIMergeEditorPlugin( UIMergeView* view ) : mView( view ) {}
	enum class LineType : Uint8 { Common, Added, Removed, Conflict };

	std::string getId() override { return "MergeEditorPlugin"; }
	std::string getTitle() override { return "MergeEditorPlugin"; }
	std::string getDescription() override { return "Highlights merge changes and conflicts."; }
	bool isReady() const override { return true; }
	void onRegister( UICodeEditor* editor ) override {
		auto* themeManager = editor->getUISceneNode()->getUIThemeManager();
		Font* font = themeManager->getDefaultFont();
		mHeaderHeight =
			font ? std::ceil( themeManager->getDefaultFontSize() ) + PixelDensity::dpToPxI( 8 )
				 : PixelDensity::dpToPxI( 24 );
		editor->registerTopSpace( this, mHeaderHeight, 0 );
	}
	void onUnregister( UICodeEditor* editor ) override { editor->unregisterTopSpace( this ); }
	bool onKeyDown( UICodeEditor*, const KeyEvent& event ) override {
		return mView->executeMergeKeyBinding( event );
	}

	void setLines( std::vector<LineType> lines ) { mLines = std::move( lines ); }
	void setHeader( String label, bool present, const String& missingVersionLabel ) {
		mHeader = std::move( label );
		if ( !present && !missingVersionLabel.empty() )
			mHeader += " · " + missingVersionLabel;
	}

	void drawTop( UICodeEditor* editor, const Vector2f& screenStart, const Sizef& size,
				  const Float& ) override {
		Primitives primitives;
		primitives.setColor(
			editor->getColorScheme().getEditorColor( SyntaxStyleTypes::Background ) );
		primitives.drawRectangle( Rectf( screenStart, size ) );
		primitives.setColor(
			editor->getColorScheme().getEditorColor( SyntaxStyleTypes::LineBreakColumn ) );
		primitives.drawRectangle(
			{ { screenStart.x, screenStart.y + size.y - PixelDensity::dpToPx( 1 ) },
			  { size.x, PixelDensity::dpToPx( 1 ) } } );

		auto* themeManager = editor->getUISceneNode()->getUIThemeManager();
		Font* font = themeManager->getDefaultFont();
		if ( !font || mHeader.empty() )
			return;
		const Float fontSize = themeManager->getDefaultFontSize();
		const Float offsetY = eefloor( ( size.y - font->getLineSpacing( fontSize ) ) * 0.5f );
		Text::draw( mHeader, { screenStart.x + PixelDensity::dpToPx( 8 ), screenStart.y + offsetY },
					font, fontSize,
					editor->getColorScheme().getEditorColor( SyntaxStyleTypes::LineNumber2 ) );
	}

	void drawBeforeLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							 const Float&, const Float& lineHeight ) override {
		if ( index < 0 || index >= static_cast<Int64>( mLines.size() ) ||
			 mLines[index] == LineType::Common )
			return;
		Color color;
		switch ( mLines[index] ) {
			case LineType::Added:
				color = Color( 0, 150, 32, 40 );
				break;
			case LineType::Removed:
				color = Color( 180, 0, 32, 40 );
				break;
			case LineType::Conflict:
				color = Color( 180, 0, 32, 64 );
				break;
			case LineType::Common:
				return;
		}
		Primitives primitives;
		primitives.setColor( color );
		primitives.drawRectangle( { { editor->getScreenPos().x, position.y },
									{ editor->getPixelsSize().x, lineHeight } } );
	}

  private:
	UIMergeView* mView;
	std::vector<LineType> mLines;
	String mHeader;
	Float mHeaderHeight{ 0 };
};

UIMergeView* UIMergeView::New() {
	return eeNew( UIMergeView, () );
}

UIMergeView::UIMergeView() :
	UILinearLayout( "mergeview", UIOrientation::Vertical ),
	WidgetCommandExecuter( KeyBindings{ getUISceneNode()->getWindow()->getInput() } ) {
	setFlags( UI_AUTO_SIZE );
	createToolbar();
	mEditorsLayout = UILinearLayout::NewHorizontal();
	mEditorsLayout->setParent( this );
	mEditorsLayout->setId( "merge_editors" );
	mEditorsLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );
	mEditorsLayout->setLayoutWeight( 1 );
	for ( auto** editor : { &mLeftEditor, &mResultEditor, &mRightEditor } ) {
		*editor = UICodeEditor::New();
		( *editor )->setParent( mEditorsLayout );
		( *editor )->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::MatchParent );
		( *editor )->setLayoutWeight( 1 );
		( *editor )->setShowFoldingRegion( false );
	}
	mLeftEditor->setLocked( true );
	mRightEditor->setLocked( true );
	// The right editor owns the single visible vertical scrollbar for the three panes.
	mLeftEditor->setVerticalScrollBarEnabled( false );
	mResultEditor->setVerticalScrollBarEnabled( false );
	mLeftPlugin = std::make_unique<UIMergeEditorPlugin>( this );
	mResultPlugin = std::make_unique<UIMergeEditorPlugin>( this );
	mRightPlugin = std::make_unique<UIMergeEditorPlugin>( this );
	mLeftEditor->registerPlugin( mLeftPlugin.get() );
	mResultEditor->registerPlugin( mResultPlugin.get() );
	mRightEditor->registerPlugin( mRightPlugin.get() );
	mResultEditor->on( Event::OnTextChanged, [this]( const Event* ) {
		if ( mApplyingBlock )
			return;
		debounce(
			[this] {
				refreshBlocks();
				refreshHighlights();
			},
			Milliseconds( 100 ), String::hash( "merge-view-refresh" ) );
	} );

	for ( auto* source : { mLeftEditor, mResultEditor, mRightEditor } ) {
		source->on( Event::OnScrollChange,
					[this, source]( const Event* ) { syncScroll( source ); } );
		source->getVScrollBar()->on( Event::OnValueChange,
									 [this, source]( const Event* ) { syncScroll( source ); } );
	}
}

void UIMergeView::syncScroll( UICodeEditor* source ) {
	if ( mSyncingScroll )
		return;
	mSyncingScroll = true;
	const Float sourceMaxY = source->getMaxScroll().y;
	const Float scrollRatio = sourceMaxY > 0 ? source->getScroll().y / sourceMaxY : 0;
	for ( auto* target : { mLeftEditor, mResultEditor, mRightEditor } ) {
		if ( target != source ) {
			target->setScrollY( scrollRatio * target->getMaxScroll().y, false );
			target->setScrollX( source->getScroll().x, false );
		}
	}
	mSyncingScroll = false;
}

void UIMergeView::createToolbar() {
	static constexpr auto TOOLBAR_LAYOUT = R"xml(
	<style>
	mergeview .mergeview_toolbar_button {
		margin: 0dp 4dp 4dp 0dp;
		padding: 2dp 4dp 2dp 4dp;
	}
	</style>
	<StackLayout id="merge_toolbar" lw="mp" lh="wc" padding="4dp 4dp 0dp 4dp">
		<PushButton id="merge_previous_conflict" text="Previous Conflict" icon="icon(arrow-up, 12dp)" class="mergeview_toolbar_button" />
		<PushButton id="merge_accept_left" text="Accept Left" icon="icon(arrow-left, 12dp)" class="mergeview_toolbar_button" />
		<PushButton id="merge_accept_both" text="Accept Both" icon="icon(arrow-both, 12dp)" class="mergeview_toolbar_button" />
		<PushButton id="merge_accept_right" text="Accept Right" icon="icon(arrow-right, 12dp)" class="mergeview_toolbar_button" />
		<PushButton id="merge_recreate_conflict" text="Recreate Conflict" icon="icon(refresh, 12dp)" class="mergeview_toolbar_button" />
		<PushButton id="merge_next_conflict" text="Next Conflict" icon="icon(arrow-down, 12dp)" class="mergeview_toolbar_button" />
	</StackLayout>
	)xml";
	getUISceneNode()->loadLayoutFromString( TOOLBAR_LAYOUT, this,
											String::hash( "uimergeview_toolbar" ) );
	mToolbar = find<UIStackLayout>( "merge_toolbar" );

	const auto bindAction = [this]( const char* buttonId, const char* command,
									const KeyBindings::Shortcut& shortcut,
									CommandCallback callback ) {
		setCommand( command, std::move( callback ) );
		if ( !shortcut.empty() )
			getKeyBindings().addKeybind( shortcut, command );
		auto* button = find<UIPushButton>( buttonId );
		const auto keybind = getKeyBindings().getCommandKeybindString( command );
		if ( !keybind.empty() )
			button->setTooltipText( button->getText() + " (" + keybind + ")" );
		button->onClick(
			[this, command = std::string{ command }]( const Event* ) { execute( command ); } );
	};
	bindAction( "merge_previous_conflict", "merge-previous-conflict", { KEY_F7 },
				[this] { goToPreviousConflict(); } );
	bindAction( "merge_accept_left", "merge-accept-left",
				{ KEY_1, KeyMod::getDefaultSecondaryModifier() },
				[this] { acceptCurrentStage2(); } );
	bindAction( "merge_accept_both", "merge-accept-both",
				{ KEY_2, KeyMod::getDefaultSecondaryModifier() }, [this] { acceptCurrentBoth(); } );
	bindAction( "merge_accept_right", "merge-accept-right",
				{ KEY_3, KeyMod::getDefaultSecondaryModifier() },
				[this] { acceptCurrentStage3(); } );
	bindAction( "merge_recreate_conflict", "merge-recreate-conflict", {}, [this] {
		if ( mRecreateConflictCallback )
			mRecreateConflictCallback();
		else
			recreateConflict();
	} );
	bindAction( "merge_next_conflict", "merge-next-conflict", { KEY_F8 },
				[this] { goToNextConflict(); } );
}

UIPushButton* UIMergeView::addToolbarAction( const std::string& command, const String& text,
											 const KeyBindings::Shortcut& shortcut,
											 const std::string& icon, CommandCallback callback ) {
	setCommand( command, std::move( callback ) );
	if ( !shortcut.empty() )
		getKeyBindings().addKeybind( shortcut, command );
	auto* button = UIPushButton::New();
	button->setParent( mToolbar );
	button->setId( command );
	button->setText( text );
	if ( auto* toolbarIcon = getUISceneNode()->findIcon( icon ) )
		button->setIcon( toolbarIcon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
	button->setClass( "mergeview_toolbar_button" );
	button->setLayoutMarginRight( PixelDensity::dpToPx( 4 ) );
	const auto keybind = getKeyBindings().getCommandKeybindString( command );
	button->setTooltipText( keybind.empty() ? text : text + " (" + keybind + ")" );
	button->onClick( [this, command]( const Event* ) { execute( command ); } );
	return button;
}

UIMergeView::~UIMergeView() {
	if ( mLeftEditor && mLeftPlugin )
		mLeftEditor->unregisterPlugin( mLeftPlugin.get() );
	if ( mResultEditor && mResultPlugin )
		mResultEditor->unregisterPlugin( mResultPlugin.get() );
	if ( mRightEditor && mRightPlugin )
		mRightEditor->unregisterPlugin( mRightPlugin.get() );
}

Uint32 UIMergeView::getType() const {
	return UI_TYPE_MERGE_VIEW;
}

bool UIMergeView::isType( const Uint32& type ) const {
	return type == getType() || UILinearLayout::isType( type );
}

void UIMergeView::load( MergeInput input ) {
	mInput = std::move( input );
	mLeftPlugin->setHeader( mInput.stage2.label, mInput.stage2.present,
							mInput.missingVersionLabel );
	mResultPlugin->setHeader( mInput.resultLabel, true, mInput.missingVersionLabel );
	mRightPlugin->setHeader( mInput.stage3.label, mInput.stage3.present,
							 mInput.missingVersionLabel );
	if ( mInput.resultDocument )
		mResultEditor->setDocument( mInput.resultDocument );
	const auto& syntaxDefinition = mResultEditor->getDocument().getSyntaxDefinition();
	const auto makeSideDocument = [&syntaxDefinition]( const MergeVersion& version ) {
		auto document = std::make_shared<Doc::TextDocument>();
		if ( version.present )
			document->textInput( version.text );
		document->setSyntaxDefinition( syntaxDefinition );
		document->setSelection( { 0, 0 } );
		return document;
	};
	mLeftEditor->setDocument( makeSideDocument( mInput.stage2 ) );
	mRightEditor->setDocument( makeSideDocument( mInput.stage3 ) );
	mOriginalResultText = mResultEditor->getDocument().getText();
	refreshBlocks();
	refreshHighlights();
}

void UIMergeView::refreshHighlights() {
	using LineType = UIMergeEditorPlugin::LineType;
	const auto& leftDocument = mLeftEditor->getDocument();
	const auto& resultDocument = mResultEditor->getDocument();
	const auto& rightDocument = mRightEditor->getDocument();
	Doc::TextDocument::ScopedReadLock leftLock( leftDocument );
	Doc::TextDocument::ScopedReadLock resultLock( resultDocument );
	Doc::TextDocument::ScopedReadLock rightLock( rightDocument );
	const auto lineViews = []( const Doc::TextDocument& document ) {
		const size_t lineCount = document.linesCount();
		std::vector<String::View> lines;
		lines.reserve( lineCount );
		for ( size_t line = 0; line < lineCount; ++line )
			lines.emplace_back( document.line( line ).getTextViewWithoutNewLine() );
		return lines;
	};
	const auto leftLines = lineViews( leftDocument );
	const auto resultLines = lineViews( resultDocument );
	const auto rightLines = lineViews( rightDocument );
	std::vector<LineType> leftTypes( leftLines.size(), LineType::Common );
	std::vector<LineType> resultTypes( resultLines.size(), LineType::Common );
	std::vector<LineType> rightTypes( rightLines.size(), LineType::Common );
	const auto mergeType = []( LineType& current, LineType type ) {
		if ( current == LineType::Common )
			current = type;
		else if ( current != type )
			current = LineType::Conflict;
	};
	const auto computeDiff = [&mergeType]( const std::vector<String::View>& oldLines,
										   const std::vector<String::View>& newLines,
										   std::vector<LineType>& oldTypes,
										   std::vector<LineType>& newTypes ) {
		dtl::Diff<String::View> diff( oldLines, newLines );
		diff.compose();
		size_t oldLine = 0;
		size_t newLine = 0;
		for ( const auto& entry : diff.getSes().getSequence() ) {
			switch ( entry.second.type ) {
				case dtl::SES_COMMON:
					++oldLine;
					++newLine;
					break;
				case dtl::SES_DELETE:
					if ( oldLine < oldTypes.size() )
						mergeType( oldTypes[oldLine], LineType::Removed );
					++oldLine;
					break;
				case dtl::SES_ADD:
					if ( newLine < newTypes.size() )
						mergeType( newTypes[newLine], LineType::Added );
					++newLine;
					break;
			}
		}
	};
	computeDiff( leftLines, resultLines, leftTypes, resultTypes );
	computeDiff( resultLines, rightLines, resultTypes, rightTypes );

	for ( const auto& block : mBlocks ) {
		const Int64 start = std::max<Int64>( 0, block.range.start().line() );
		const Int64 end = std::min<Int64>( resultTypes.size(), block.range.end().line() );
		for ( Int64 line = start; line < end; ++line )
			resultTypes[line] = LineType::Conflict;
	}
	mLeftPlugin->setLines( std::move( leftTypes ) );
	mResultPlugin->setLines( std::move( resultTypes ) );
	mRightPlugin->setLines( std::move( rightTypes ) );
	for ( auto* editor : { mLeftEditor, mResultEditor, mRightEditor } )
		editor->invalidateDraw();
}

std::vector<UIMergeView::ConflictBlock> UIMergeView::parseConflictBlocks( const String& text ) {
	std::vector<ConflictBlock> blocks;
	const auto markerWidth = []( std::string_view line, char marker ) {
		size_t width = 0;
		while ( width < line.size() && line[width] == marker )
			++width;
		return width >= 7 ? width : size_t{ 0 };
	};
	enum class ParseState { Outside, Stage2, Base, Stage3 };
	ParseState state = ParseState::Outside;
	size_t lineNumber = 0;
	size_t start = 0;
	size_t width = 0;
	std::string stage2;
	std::string stage3;
	bool firstStage2Line = true;
	bool firstStage3Line = true;
	const auto appendLine = []( std::string& destination, bool& firstLine, std::string_view line ) {
		if ( !firstLine )
			destination += '\n';
		destination.append( line.data(), line.size() );
		firstLine = false;
	};
	const auto processLine = [&]( std::string_view line ) {
		switch ( state ) {
			case ParseState::Outside:
				width = markerWidth( line, '<' );
				if ( width != 0 ) {
					start = lineNumber;
					stage2.clear();
					stage3.clear();
					firstStage2Line = true;
					firstStage3Line = true;
					state = ParseState::Stage2;
				}
				break;
			case ParseState::Stage2:
				if ( markerWidth( line, '|' ) == width )
					state = ParseState::Base;
				else if ( markerWidth( line, '=' ) == width )
					state = ParseState::Stage3;
				else
					appendLine( stage2, firstStage2Line, line );
				break;
			case ParseState::Base:
				if ( markerWidth( line, '=' ) == width )
					state = ParseState::Stage3;
				break;
			case ParseState::Stage3:
				if ( markerWidth( line, '>' ) == width ) {
					ConflictBlock block;
					block.range = { { static_cast<Int64>( start ), 0 },
									{ static_cast<Int64>( lineNumber + 1 ), 0 } };
					block.stage2 = String::fromUtf8( stage2 );
					block.stage3 = String::fromUtf8( stage3 );
					blocks.emplace_back( std::move( block ) );
					state = ParseState::Outside;
				} else {
					appendLine( stage3, firstStage3Line, line );
				}
				break;
		}
	};
	const std::string utf8Text = text.toUtf8();
	bool processedCurrentLine = false;
	String::splitCb(
		[&]( std::string_view token ) {
			if ( token == "\n" ) {
				if ( !processedCurrentLine )
					processLine( {} );
				processedCurrentLine = false;
				++lineNumber;
			} else {
				processLine( token );
				processedCurrentLine = true;
			}
			return true;
		},
		utf8Text, "", "\n", "" );
	return blocks;
}

void UIMergeView::refreshBlocks() {
	mBlocks = mResultEditor->hasDocument()
				  ? parseConflictBlocks( mResultEditor->getDocument().getText() )
				  : std::vector<ConflictBlock>{};
	if ( mCurrentBlock >= mBlocks.size() )
		mCurrentBlock = mBlocks.empty() ? 0 : mBlocks.size() - 1;
}

bool UIMergeView::hasUnresolvedMarkerBlocks() const {
	return !mBlocks.empty();
}

void UIMergeView::recreateConflict() {
	if ( !mResultEditor->hasDocument() ||
		 mResultEditor->getDocument().getText() == mOriginalResultText )
		return;
	auto& document = mResultEditor->getDocument();
	document.selectAll();
	mApplyingBlock = true;
	document.textInput( mOriginalResultText );
	mApplyingBlock = false;
	refreshBlocks();
	refreshHighlights();
}

void UIMergeView::goToNextConflict() {
	refreshBlocks();
	if ( mBlocks.empty() )
		return;
	const Int64 line = mResultEditor->getDocument().getSelection().start().line();
	auto next = std::find_if( mBlocks.begin(), mBlocks.end(), [line]( const auto& block ) {
		return block.range.start().line() > line;
	} );
	goToConflict( next == mBlocks.end() ? 0 : std::distance( mBlocks.begin(), next ) );
}

void UIMergeView::goToPreviousConflict() {
	refreshBlocks();
	if ( mBlocks.empty() )
		return;
	const Int64 line = mResultEditor->getDocument().getSelection().start().line();
	auto previous = std::find_if( mBlocks.rbegin(), mBlocks.rend(), [line]( const auto& block ) {
		return block.range.start().line() < line;
	} );
	goToConflict( previous == mBlocks.rend() ? mBlocks.size() - 1
											 : std::distance( previous, mBlocks.rend() ) - 1 );
}

void UIMergeView::goToConflict( size_t block ) {
	if ( block >= mBlocks.size() )
		return;
	mCurrentBlock = block;
	const TextPosition position = mBlocks[block].range.start();
	mResultEditor->getDocument().setSelection( position );
	mResultEditor->goToLine( position );
	mResultEditor->setFocus();
}

size_t UIMergeView::getCurrentConflictBlock() {
	const TextPosition cursor = mResultEditor->getDocument().getSelection().start();
	auto block = std::find_if( mBlocks.begin(), mBlocks.end(), [&cursor]( const auto& conflict ) {
		return conflict.range.contains( cursor );
	} );
	if ( block != mBlocks.end() )
		mCurrentBlock = std::distance( mBlocks.begin(), block );
	return std::min( mCurrentBlock, mBlocks.size() - 1 );
}

void UIMergeView::acceptCurrentStage2() {
	refreshBlocks();
	if ( !mBlocks.empty() )
		acceptStage2( getCurrentConflictBlock() );
}

void UIMergeView::acceptCurrentStage3() {
	refreshBlocks();
	if ( !mBlocks.empty() )
		acceptStage3( getCurrentConflictBlock() );
}

void UIMergeView::acceptCurrentBoth() {
	refreshBlocks();
	if ( !mBlocks.empty() )
		acceptBoth( getCurrentConflictBlock() );
}

void UIMergeView::replaceBlock( size_t block, const String& replacement ) {
	if ( block >= mBlocks.size() )
		return;
	auto& document = mResultEditor->getDocument();
	const TextRange range = mBlocks[block].range;
	String text( replacement );
	if ( !text.empty() && text.back() != '\n' &&
		 range.end().line() < static_cast<Int64>( document.linesCount() ) )
		text += '\n';
	document.setSelection( range );
	mApplyingBlock = true;
	document.textInput( text );
	mApplyingBlock = false;
	refreshBlocks();
	refreshHighlights();
}

void UIMergeView::acceptStage2( size_t block ) {
	refreshBlocks();
	if ( block < mBlocks.size() )
		replaceBlock( block, String( mBlocks[block].stage2 ) );
}

void UIMergeView::acceptStage3( size_t block ) {
	refreshBlocks();
	if ( block < mBlocks.size() )
		replaceBlock( block, String( mBlocks[block].stage3 ) );
}

void UIMergeView::acceptBoth( size_t block, Order order ) {
	refreshBlocks();
	if ( block >= mBlocks.size() )
		return;
	const auto& first =
		order == Order::Stage2ThenStage3 ? mBlocks[block].stage2 : mBlocks[block].stage3;
	const auto& second =
		order == Order::Stage2ThenStage3 ? mBlocks[block].stage3 : mBlocks[block].stage2;
	replaceBlock( block, first + ( first.empty() || second.empty() ? "" : "\n" ) + second );
}

void UIMergeView::setSyntaxColorScheme( const SyntaxColorScheme& colorScheme ) {
	for ( auto* editor : { mLeftEditor, mResultEditor, mRightEditor } )
		editor->setColorScheme( colorScheme );
}

void UIMergeView::setToolbarVisible( bool visible ) {
	if ( mToolbarVisible == visible )
		return;
	mToolbarVisible = visible;
	mToolbar->setVisible( visible );
}

Uint32 UIMergeView::onKeyDown( const KeyEvent& event ) {
	const Uint32 handled = executeMergeKeyBinding( event );
	return handled ? handled : UIWidget::onKeyDown( event );
}

bool UIMergeView::executeMergeKeyBinding( const KeyEvent& event ) {
	return WidgetCommandExecuter::onKeyDown( event ) != 0;
}

} // namespace EE::UI::Tools
