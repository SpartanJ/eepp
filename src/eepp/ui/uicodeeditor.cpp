#include <algorithm>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/tools/uicolorpicker.hpp>
#include <eepp/ui/tools/uidocfindreplace.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

using namespace EE::UI::Tools;

namespace EE { namespace UI {

UICodeEditor* UICodeEditor::New() {
	return eeNew( UICodeEditor, ( true, true ) );
}

UICodeEditor* UICodeEditor::NewOpt( const bool& autoRegisterBaseCommands,
									const bool& autoRegisterBaseKeybindings ) {
	return eeNew( UICodeEditor, ( autoRegisterBaseCommands, autoRegisterBaseKeybindings ) );
}

const std::map<KeyBindings::Shortcut, std::string> UICodeEditor::getDefaultKeybindings() {
	return {
		{ { KEY_BACKSPACE, KeyMod::getDefaultModifier() }, "delete-to-previous-word" },
		{ { KEY_BACKSPACE, KEYMOD_SHIFT }, "delete-to-previous-char" },
		{ { KEY_BACKSPACE, 0 }, "delete-to-previous-char" },
		{ { KEY_DELETE, KeyMod::getDefaultModifier() }, "delete-to-next-word" },
		{ { KEY_DELETE, KEYMOD_SHIFT }, "delete-current-line" },
		{ { KEY_DELETE, 0 }, "delete-to-next-char" },
		{ { KEY_KP_ENTER, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "new-line-above" },
		{ { KEY_RETURN, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "new-line-above" },
		{ { KEY_KP_ENTER, KeyMod::getDefaultModifier() }, "new-line" },
		{ { KEY_RETURN, KeyMod::getDefaultModifier() }, "new-line" },
		{ { KEY_KP_ENTER, KEYMOD_SHIFT }, "new-line" },
		{ { KEY_RETURN, KEYMOD_SHIFT }, "new-line" },
		{ { KEY_KP_ENTER, 0 }, "new-line" },
		{ { KEY_RETURN, 0 }, "new-line" },
		{ { KEY_UP, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "move-lines-up" },
		{ { KEY_UP, KeyMod::getDefaultModifier() }, "move-scroll-up" },
		{ { KEY_UP, KEYMOD_SHIFT }, "select-to-previous-line" },
		{ { KEY_UP, 0 }, "move-to-previous-line" },
		{ { KEY_DOWN, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "move-lines-down" },
		{ { KEY_DOWN, KeyMod::getDefaultModifier() }, "move-scroll-down" },
		{ { KEY_DOWN, KEYMOD_SHIFT }, "select-to-next-line" },
		{ { KEY_DOWN, 0 }, "move-to-next-line" },
		{ { KEY_LEFT, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-previous-word" },
		{ { KEY_LEFT, KeyMod::getDefaultModifier() }, "move-to-previous-word" },
		{ { KEY_LEFT, KEYMOD_SHIFT }, "select-to-previous-char" },
		{ { KEY_LEFT, 0 }, "move-to-previous-char" },
		{ { KEY_RIGHT, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-next-word" },
		{ { KEY_RIGHT, KeyMod::getDefaultModifier() }, "move-to-next-word" },
		{ { KEY_RIGHT, KEYMOD_SHIFT }, "select-to-next-char" },
		{ { KEY_RIGHT, 0 }, "move-to-next-char" },
		{ { KEY_Z, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "redo" },
		{ { KEY_HOME, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-start-of-doc" },
		{ { KEY_HOME, KEYMOD_SHIFT }, "select-to-start-of-content" },
		{ { KEY_HOME, KeyMod::getDefaultModifier() }, "move-to-start-of-doc" },
		{ { KEY_HOME, 0 }, "move-to-start-of-content" },
		{ { KEY_END, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-end-of-doc" },
		{ { KEY_END, KEYMOD_SHIFT }, "select-to-end-of-line" },
		{ { KEY_END, KeyMod::getDefaultModifier() }, "move-to-end-of-doc" },
		{ { KEY_END, 0 }, "move-to-end-of-line" },
		{ { KEY_PAGEUP, KeyMod::getDefaultModifier() }, "move-to-previous-page" },
		{ { KEY_PAGEUP, KEYMOD_SHIFT }, "select-to-previous-page" },
		{ { KEY_PAGEUP, 0 }, "move-to-previous-page" },
		{ { KEY_PAGEDOWN, KeyMod::getDefaultModifier() }, "move-to-next-page" },
		{ { KEY_PAGEDOWN, KEYMOD_SHIFT }, "select-to-next-page" },
		{ { KEY_PAGEDOWN, 0 }, "move-to-next-page" },
		{ { KEY_Y, KeyMod::getDefaultModifier() }, "redo" },
		{ { KEY_Z, KeyMod::getDefaultModifier() }, "undo" },
		{ { KEY_TAB, KEYMOD_SHIFT }, "unindent" },
		{ { KEY_TAB, 0 }, "indent" },
		{ { KEY_C, KeyMod::getDefaultModifier() }, "copy" },
		{ { KEY_X, KeyMod::getDefaultModifier() }, "cut" },
		{ { KEY_V, KeyMod::getDefaultModifier() }, "paste" },
		{ { KEY_INSERT, KEYMOD_SHIFT }, "paste" },
		{ { KEY_A, KeyMod::getDefaultModifier() }, "select-all" },
		{ { KEY_PLUS, KeyMod::getDefaultModifier() }, "font-size-grow" },
		{ { KEY_KP_PLUS, KeyMod::getDefaultModifier() }, "font-size-grow" },
		{ { KEY_MINUS, KeyMod::getDefaultModifier() }, "font-size-shrink" },
		{ { KEY_KP_MINUS, KeyMod::getDefaultModifier() }, "font-size-shrink" },
		{ { KEY_0, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "font-size-reset" },
		{ { KEY_KP_DIVIDE, KeyMod::getDefaultModifier() }, "toggle-line-comments" },
		{ { KEY_UP, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "selection-to-upper" },
		{ { KEY_DOWN, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "selection-to-lower" },
		{ { KEY_F, KeyMod::getDefaultModifier() }, "find-replace" },
		{ { KEY_D, KeyMod::getDefaultModifier() }, "select-word" },
		{ { KEY_UP, KEYMOD_LALT }, "add-cursor-above" },
		{ { KEY_DOWN, KEYMOD_LALT }, "add-cursor-below" },
		{ { KEY_ESCAPE }, "reset-cursor" },
		{ { KEY_U, KeyMod::getDefaultModifier() }, "cursor-undo" },
		{ { KEY_A, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-all-matches" },
		{ { KEY_APPLICATION }, "open-context-menu" },
	};
}

UICodeEditor::UICodeEditor( const std::string& elementTag, const bool& autoRegisterBaseCommands,
							const bool& autoRegisterBaseKeybindings ) :
	UIWidget( elementTag ),
	mFont( FontManager::instance()->getByName( "monospace" ) ),
	mDoc( std::make_shared<TextDocument>() ),
	mBlinkTime( Seconds( 0.5f ) ),
	mTabWidth( 4 ),
	mMouseWheelScroll( 50 ),
	mFontSize( mFontStyleConfig.getFontCharacterSize() ),
	mLineNumberPaddingLeft( 8 ),
	mLineNumberPaddingRight( 8 ),
	mKeyBindings( getUISceneNode()->getWindow()->getInput() ),
	mFindLongestLineWidthUpdateFrequency( Seconds( 1 ) ),
	mPreviewColor( Color::Transparent ) {
	mFlags |= UI_TAB_STOP | UI_OWNS_CHILDS_POSITION;
	setTextSelection( true );
	setColorScheme( SyntaxColorScheme::getDefault() );
	mVScrollBar = UIScrollBar::NewVertical();
	mVScrollBar->setParent( this );
	mVScrollBar->addEventListener( Event::OnSizeChange,
								   [&]( const Event* ) { updateScrollBar(); } );
	mVScrollBar->addEventListener( Event::OnValueChange, [&]( const Event* ) {
		setScrollY( mVScrollBar->getValue() * getMaxScroll().y, false );
	} );

	mHScrollBar = UIScrollBar::NewHorizontal();
	mHScrollBar->setParent( this );
	mHScrollBar->addEventListener( Event::OnSizeChange,
								   [&]( const Event* ) { updateScrollBar(); } );
	mHScrollBar->addEventListener( Event::OnValueChange, [&]( const Event* ) {
		setScrollX( mHScrollBar->getValue() * getMaxScroll().x, false );
	} );

	if ( NULL == mFont && elementTag == "codeeditor" )
		Log::error(
			"A monospace font must be loaded to be able to use the code editor.\nTry loading "
			"a font with the name \"monospace\"" );

	mFontStyleConfig.Font = mFont;

	setFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );

	setClipType( ClipType::ContentBox );
	mDoc->registerClient( this );
	subscribeScheduledUpdate();

	if ( autoRegisterBaseCommands )
		registerCommands();
	if ( autoRegisterBaseKeybindings )
		registerKeybindings();
}

UICodeEditor::UICodeEditor( const bool& autoRegisterBaseCommands,
							const bool& autoRegisterBaseKeybindings ) :
	UICodeEditor( "codeeditor", autoRegisterBaseCommands, autoRegisterBaseKeybindings ) {}

UICodeEditor::~UICodeEditor() {
	if ( mCurrentMenu ) {
		mCurrentMenu->clearEventListener();
		mCurrentMenu = nullptr;
	}

	for ( auto& plugin : mPlugins )
		plugin->onUnregister( this );

	// TODO: Use a condition variable to wait the thread pool to finish
	while ( mHighlightWordProcessing )
		Sys::sleep( Milliseconds( 1 ) );

	if ( mDoc.use_count() == 1 ) {
		DocEvent event( this, mDoc.get(), Event::OnDocumentClosed );
		sendEvent( &event );
		mDoc->unregisterClient( this );
		mDoc.reset();
	} else {
		mDoc->unregisterClient( this );
	}
}

Uint32 UICodeEditor::getType() const {
	return UI_TYPE_CODEEDITOR;
}

bool UICodeEditor::isType( const Uint32& type ) const {
	return type == getType() || UIWidget::isType( type );
}

void UICodeEditor::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "codeeditor" );
}

void UICodeEditor::draw() {
	if ( !mVisible || mAlpha == 0 )
		return;

	UIWidget::draw();

	if ( mFont == NULL )
		return;

	if ( mDisplayLoaderIfDocumentLoading && mDoc->isLoading() ) {
		UILoader* loader = getLoader();
		loader->setParent( this );
		loader->setVisible( true );
		loader->setEnabled( false );
		loader->setPixelsSize( getPixelsSize() );
	} else if ( mLoader != nullptr && !mDoc->isLoading() && mLoader->isVisible() ) {
		mLoader->setVisible( false );
	}

	if ( mDoc->isLoading() )
		return;

	if ( mDirtyEditor )
		updateEditor();

	Color col;
	auto lineRange = getVisibleLineRange();
	Float charSize = PixelDensity::pxToDp( getCharacterSize() );
	Float lineHeight = getLineHeight();
	int lineNumberDigits = getLineNumberDigits();
	Float gutterWidth = getGutterWidth();
	Vector2f screenStart( getScreenStart() );
	Vector2f start( screenStart.x + gutterWidth, screenStart.y + getPluginsTopSpace() );
	Vector2f startScroll( start - mScroll );
	Primitives primitives;
	TextPosition cursor( mDoc->getSelection().start() );

	for ( auto& plugin : mPlugins )
		plugin->preDraw( this, startScroll, lineHeight, cursor );

	if ( mPluginsTopSpace > 0 ) {
		Float curTopPos = 0.f;
		for ( auto& plugin : mPluginTopSpaces ) {
			plugin.plugin->drawTop( this, { screenStart.x, screenStart.y + curTopPos },
									{ mSize.getWidth(), plugin.space }, charSize );
			curTopPos += plugin.space;
		}
	}

	if ( !mLocked && mHighlightCurrentLine ) {
		for ( const auto& cursor : mDoc->getSelections() ) {
			primitives.setColor( Color( mCurrentLineBackgroundColor ).blendAlpha( mAlpha ) );
			primitives.drawRectangle(
				Rectf( Vector2f( startScroll.x + mScroll.x,
								 startScroll.y + cursor.start().line() * lineHeight ),
					   Sizef( mSize.getWidth(), lineHeight ) ) );
		}
	}

	if ( mLineBreakingColumn ) {
		Float lineBreakingOffset = startScroll.x + getGlyphWidth() * mLineBreakingColumn;
		if ( lineBreakingOffset >= start.x ) {
			primitives.setColor( Color( mLineBreakColumnColor ).blendAlpha( mAlpha ) );
			primitives.drawLine( { { lineBreakingOffset, start.y },
								   { lineBreakingOffset, start.y + mSize.getHeight() } } );
		}
	}

	if ( mHighlightMatchingBracket ) {
		drawMatchingBrackets( startScroll, lineHeight );
	}

	if ( mHighlightTextRange.isValid() && mHighlightTextRange.hasSelection() ) {
		drawTextRange( mHighlightTextRange, lineRange, startScroll, lineHeight,
					   mColorScheme.getEditorSyntaxStyle( "selection_region" ).color );
	}

	if ( mHighlightSelectionMatch && mDoc->hasSelection() && mDoc->getSelection().inSameLine() ) {
		drawSelectionMatch( lineRange, startScroll, lineHeight );
	}

	if ( mDoc->hasSelection() ) {
		auto selections = mDoc->getSelectionsSorted();
		for ( const auto& sel : selections ) {
			drawTextRange( sel, lineRange, startScroll, lineHeight,
						   mFontStyleConfig.getFontSelectionBackColor() );
		}
	}

	if ( !mHighlightWord.isEmpty() )
		drawWordRanges( mHighlightWordCache, lineRange, startScroll, lineHeight, true );

	if ( mShowIndentationGuides ) {
		drawIndentationGuides( lineRange, startScroll, lineHeight );
	}

	// Draw tab marker
	if ( mShowWhitespaces ) {
		drawWhitespaces( lineRange, startScroll, lineHeight );
	}

	if ( mShowLineEndings ) {
		drawLineEndings( lineRange, startScroll, lineHeight );
	}

	for ( unsigned long i = lineRange.first; i <= lineRange.second; i++ ) {
		Vector2f curScroll(
			{ startScroll.x, static_cast<float>( startScroll.y + lineHeight * (double)i ) } );

		for ( auto& plugin : mPlugins )
			plugin->drawBeforeLineText( this, i, curScroll, charSize, lineHeight );

		drawLineText( i, curScroll, charSize, lineHeight );

		for ( auto& plugin : mPlugins )
			plugin->drawAfterLineText( this, i, curScroll, charSize, lineHeight );

		if ( mPluginsGutterSpace > 0 ) {
			Float curGutterPos = 0.f;
			for ( auto& plugin : mPluginGutterSpaces ) {
				for ( unsigned long i = lineRange.first; i <= lineRange.second; i++ ) {
					plugin.plugin->drawGutter( this, i,
											   { screenStart.x + curGutterPos, curScroll.y },
											   lineHeight, plugin.space, charSize );
				}
				curGutterPos += plugin.space;
			}
		}
	}

	for ( const auto& cursor : mDoc->getSelections() )
		drawCursor( startScroll, lineHeight, cursor.start() );

	if ( mShowLineNumber ) {
		drawLineNumbers( lineRange, startScroll,
						 { screenStart.x + mPluginsGutterSpace, screenStart.y }, lineHeight,
						 getLineNumberWidth(), lineNumberDigits, charSize );
	}

	if ( mColorPreview && mPreviewColorRange.isValid() && isMouseOver() && !mMinimapHover ) {
		drawColorPreview( startScroll, lineHeight );
	}

	if ( mMinimapEnabled )
		drawMinimap( screenStart, lineRange );

	for ( auto& plugin : mPlugins )
		plugin->postDraw( this, startScroll, lineHeight, cursor );
}

void UICodeEditor::scheduledUpdate( const Time& ) {
	if ( hasFocus() && getUISceneNode()->getWindow()->hasFocus() ) {
		if ( mBlinkTime != Time::Zero && mBlinkTimer.getElapsedTime() > mBlinkTime ) {
			mCursorVisible = !mCursorVisible;
			mBlinkTimer.restart();
			invalidateDraw();
		}
	}

	if ( mMouseDown ) {
		if ( !( getUISceneNode()->getWindow()->getInput()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
			stopMinimapDragging( getUISceneNode()->getWindow()->getInput()->getMousePosf() );
			mMouseDown = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		} else if ( !isMouseOverMeOrChilds() || mMinimapDragging ) {
			onMouseMove( getUISceneNode()->getEventDispatcher()->getMousePos(),
						 getUISceneNode()->getEventDispatcher()->getPressTrigger() );
		}
	}

	if ( !mVisible )
		return;

	if ( mDoc && !mDoc->isLoading() &&
		 mDoc->getHighlighter()->updateDirty( getVisibleLinesCount() ) ) {
		invalidateDraw();
	}

	if ( mDoc && !mDoc->isLoading() && mHorizontalScrollBarEnabled && hasFocus() &&
		 mLongestLineWidthDirty &&
		 mLongestLineWidthLastUpdate.getElapsedTime() > mFindLongestLineWidthUpdateFrequency ) {
		updateLongestLineWidth();
	}

	for ( auto& plugin : mPlugins )
		plugin->update( this );
}

void UICodeEditor::updateLongestLineWidth() {
	if ( mHorizontalScrollBarEnabled && mDoc && !mDoc->isLoading() &&
		 !mDoc->isRunningTransaction() ) {
		Float maxWidth = mLongestLineWidth;
		findLongestLine();
		mLongestLineWidthLastUpdate.restart();
		mLongestLineWidthDirty = false;
		if ( maxWidth != mLongestLineWidth )
			updateScrollBar();
	}
}

void UICodeEditor::reset() {
	mDoc->reset();
	mDoc->getHighlighter()->reset();
	invalidateDraw();
}

TextDocument::LoadStatus UICodeEditor::loadFromFile( const std::string& path ) {
	auto ret = mDoc->loadFromFile( path );
	if ( ret == TextDocument::LoadStatus::Loaded ) {
		invalidateEditor();
		updateLongestLineWidth();
		invalidateDraw();
		onDocumentLoaded( mDoc.get() );
	}
	return ret;
}

bool UICodeEditor::loadAsyncFromFile(
	const std::string& path, std::shared_ptr<ThreadPool> pool,
	std::function<void( std::shared_ptr<TextDocument>, bool )> onLoaded ) {
	bool wasLocked = isLocked();
	if ( !wasLocked )
		setLocked( true );
	bool ret = mDoc->loadAsyncFromFile( path, pool,
										[this, onLoaded, wasLocked]( TextDocument*, bool success ) {
											if ( !success ) {
												runOnMainThread( [&, onLoaded, wasLocked, success] {
													if ( !wasLocked )
														setLocked( false );
													if ( onLoaded )
														onLoaded( mDoc, success );
												} );
												return;
											}
											runOnMainThread( [&, onLoaded, wasLocked, success] {
												invalidateEditor();
												updateLongestLineWidth();
												invalidateDraw();
												if ( !wasLocked )
													setLocked( false );
												onDocumentLoaded();
												if ( onLoaded )
													onLoaded( mDoc, success );
											} );
										} );
	if ( !ret && !wasLocked )
		setLocked( false );
	return ret;
}

TextDocument::LoadStatus UICodeEditor::loadFromURL( const std::string& url,
													const Http::Request::FieldTable& headers ) {
	auto ret = mDoc->loadFromURL( url, headers );
	if ( ret == TextDocument::LoadStatus::Loaded ) {
		invalidateEditor();
		updateLongestLineWidth();
		invalidateDraw();
		onDocumentLoaded();
	}
	return ret;
}

bool UICodeEditor::loadAsyncFromURL(
	const std::string& url, const Http::Request::FieldTable& headers,
	std::function<void( std::shared_ptr<TextDocument>, bool )> onLoaded ) {
	bool wasLocked = isLocked();
	if ( !wasLocked )
		setLocked( true );
	bool ret = mDoc->loadAsyncFromURL(
		url, headers,
		[this, onLoaded, wasLocked]( TextDocument*, bool success ) {
			runOnMainThread( [&, onLoaded, wasLocked] {
				invalidateEditor();
				updateLongestLineWidth();
				invalidateDraw();
				if ( !wasLocked )
					setLocked( false );
				onDocumentLoaded();
				if ( onLoaded )
					onLoaded( mDoc, success );
			} );
		},
		[&]( const Http&, const Http::Request&, const Http::Response&,
			 const Http::Request::Status& status, size_t /*totalBytes*/, size_t /*currentBytes*/ ) {
			if ( status == Http::Request::ContentReceived ) {
				runOnMainThread( [&] { invalidateDraw(); } );
			}
			return true;
		} );
	if ( !ret && !wasLocked )
		setLocked( false );
	return ret;
}

bool UICodeEditor::save() {
	return mDoc->save();
}

bool UICodeEditor::save( const std::string& path ) {
	return mDoc->save( path );
}

bool UICodeEditor::save( IOStreamFile& stream ) {
	return mDoc->save( stream );
}

Font* UICodeEditor::getFont() const {
	return mFont;
}

const UIFontStyleConfig& UICodeEditor::getFontStyleConfig() const {
	return mFontStyleConfig;
}

UICodeEditor* UICodeEditor::setFont( Font* font ) {
	if ( mFont != font ) {
		mFont = font;
		mFontStyleConfig.Font = mFont;
		invalidateDraw();
		invalidateEditor();
		onFontChanged();
	}
	return this;
}

void UICodeEditor::onFontChanged() {
	invalidateLinesCache();
	udpateGlyphWidth();
}

void UICodeEditor::onFontStyleChanged() {
	invalidateLinesCache();
	udpateGlyphWidth();
}

void UICodeEditor::onDocumentLoaded( TextDocument* ) {}

void UICodeEditor::onDocumentLoaded() {
	DocEvent event( this, mDoc.get(), Event::OnDocumentLoaded );
	sendEvent( &event );
}

void UICodeEditor::onDocumentChanged() {
	invalidateLinesCache();
	if ( mFindReplace )
		mFindReplace->setDoc( mDoc );
	DocEvent event( this, mDoc.get(), Event::OnDocumentChanged );
	sendEvent( &event );
}

Uint32 UICodeEditor::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::MouseDown )
		return 1;
	return UIWidget::onMessage( msg );
}

void UICodeEditor::disableEditorFeatures() {
	mShowLineNumber = false;
	mShowWhitespaces = false;
	mHighlightCurrentLine = false;
	mHighlightMatchingBracket = false;
	mHighlightSelectionMatch = false;
	mEnableColorPickerOnSelection = false;
	mMinimapEnabled = false;
	mFindReplaceEnabled = false;
	mLineBreakingColumn = 0;
}

Float UICodeEditor::getViewportWidth( const bool& forceVScroll ) const {
	Float vScrollWidth =
		mVScrollBar->isVisible() || forceVScroll ? mVScrollBar->getPixelsSize().getWidth() : 0.f;
	if ( mMinimapEnabled )
		vScrollWidth += getMinimapWidth();
	Float viewWidth = eefloor( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right -
							   getGutterWidth() - vScrollWidth );
	return viewWidth;
}

bool UICodeEditor::getShowIndentationGuides() const {
	return mShowIndentationGuides;
}

void UICodeEditor::setShowIndentationGuides( bool showIndentationGuides ) {
	if ( showIndentationGuides != mShowIndentationGuides ) {
		mShowIndentationGuides = showIndentationGuides;
		invalidateDraw();
	}
}

UICodeEditor* UICodeEditor::setFontSize( const Float& dpSize ) {
	if ( mFontStyleConfig.CharacterSize != dpSize ) {
		mFontStyleConfig.CharacterSize =
			eeabs( dpSize - (int)dpSize ) == 0.5f || (int)dpSize == dpSize ? dpSize
																		   : eefloor( dpSize );
		mFontSize = mFontStyleConfig.CharacterSize;
		udpateGlyphWidth();
		invalidateDraw();
		onFontChanged();
	}
	return this;
}

const Float& UICodeEditor::getFontSize() const {
	return mFontStyleConfig.getFontCharacterSize();
}

UICodeEditor* UICodeEditor::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.getFontColor() != color ) {
		mFontStyleConfig.FontColor = color;
		invalidateDraw();
		onFontStyleChanged();
	}
	return this;
}

const Color& UICodeEditor::getFontColor() const {
	return mFontStyleConfig.getFontColor();
}

const Color& UICodeEditor::getFontSelectedColor() const {
	return mFontStyleConfig.getFontSelectedColor();
}

UICodeEditor* UICodeEditor::setFontSelectedColor( const Color& color ) {
	if ( mFontStyleConfig.getFontSelectedColor() != color ) {
		mFontStyleConfig.FontSelectedColor = color;
		invalidateDraw();
		onFontStyleChanged();
	}
	return this;
}

UICodeEditor* UICodeEditor::setFontSelectionBackColor( const Color& color ) {
	if ( mFontStyleConfig.getFontSelectionBackColor() != color ) {
		mFontStyleConfig.FontSelectionBackColor = color;
		invalidateDraw();
		onFontStyleChanged();
	}
	return this;
}

const Color& UICodeEditor::getFontSelectionBackColor() const {
	return mFontStyleConfig.getFontSelectionBackColor();
}

const Uint32& UICodeEditor::getTabWidth() const {
	return mTabWidth;
}

UICodeEditor* UICodeEditor::setTabWidth( const Uint32& tabWidth ) {
	mTabWidth = tabWidth;
	return this;
}

const Float& UICodeEditor::getMouseWheelScroll() const {
	return mMouseWheelScroll;
}

void UICodeEditor::setMouseWheelScroll( const Float& mouseWheelScroll ) {
	mMouseWheelScroll = mouseWheelScroll;
}

void UICodeEditor::setLineNumberPaddingLeft( const Float& dpLeft ) {
	if ( dpLeft != mLineNumberPaddingLeft ) {
		mLineNumberPaddingLeft = dpLeft;
		invalidateDraw();
	}
}

void UICodeEditor::setLineNumberPaddingRight( const Float& dpRight ) {
	if ( dpRight != mLineNumberPaddingRight ) {
		mLineNumberPaddingRight = dpRight;
		invalidateDraw();
	}
}

void UICodeEditor::setLineNumberPadding( const Float& dpPaddingLeft, const Float& dpPaddingRight ) {
	setLineNumberPaddingLeft( dpPaddingLeft );
	setLineNumberPaddingRight( dpPaddingRight );
}

const Float& UICodeEditor::getLineNumberPaddingLeft() const {
	return mLineNumberPaddingLeft;
}

const Float& UICodeEditor::getLineNumberPaddingRight() const {
	return mLineNumberPaddingRight;
}

size_t UICodeEditor::getLineNumberDigits() const {
	return eemax<size_t>( 2UL, Math::countDigits( mDoc->linesCount() ) );
}

Float UICodeEditor::getLineNumberWidth() const {
	return mShowLineNumber ? eeceil( getLineNumberDigits() * getGlyphWidth() +
									 getLineNumberPaddingLeft() + getLineNumberPaddingRight() )
						   : 0.f;
}

Float UICodeEditor::getGutterWidth() const {
	return getLineNumberWidth() + mPluginsGutterSpace;
}

const bool& UICodeEditor::getShowLineNumber() const {
	return mShowLineNumber;
}

void UICodeEditor::setShowLineNumber( const bool& showLineNumber ) {
	if ( mShowLineNumber != showLineNumber ) {
		mShowLineNumber = showLineNumber;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getLineNumberBackgroundColor() const {
	return mLineNumberBackgroundColor;
}

void UICodeEditor::setLineNumberBackgroundColor( const Color& lineNumberBackgroundColor ) {
	if ( mLineNumberBackgroundColor != lineNumberBackgroundColor ) {
		mLineNumberBackgroundColor = lineNumberBackgroundColor;
		invalidateDraw();
	};
}

const Color& UICodeEditor::getCurrentLineBackgroundColor() const {
	return mCurrentLineBackgroundColor;
}

void UICodeEditor::setCurrentLineBackgroundColor( const Color& currentLineBackgroundColor ) {
	mCurrentLineBackgroundColor = currentLineBackgroundColor;
}

const Color& UICodeEditor::getCaretColor() const {
	return mCaretColor;
}

void UICodeEditor::setCaretColor( const Color& caretColor ) {
	if ( mCaretColor != caretColor ) {
		mCaretColor = caretColor;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getWhitespaceColor() const {
	return mWhitespaceColor;
}

void UICodeEditor::setWhitespaceColor( const Color& color ) {
	if ( mWhitespaceColor != color ) {
		mWhitespaceColor = color;
		invalidateDraw();
	}
}

const SyntaxColorScheme& UICodeEditor::getColorScheme() const {
	return mColorScheme;
}

void UICodeEditor::updateColorScheme() {
	setBackgroundColor( mColorScheme.getEditorColor( "background" ) );
	setFontColor( mColorScheme.getEditorColor( "text" ) );
	mFontStyleConfig.setFontSelectionBackColor( mColorScheme.getEditorColor( "selection" ) );
	mLineNumberFontColor = mColorScheme.getEditorColor( "line_number" );
	mLineNumberActiveFontColor = mColorScheme.getEditorColor( "line_number2" );
	mLineNumberBackgroundColor = mColorScheme.getEditorColor( "gutter_background" );
	mCurrentLineBackgroundColor = mColorScheme.getEditorColor( "line_highlight" );
	mCaretColor = mColorScheme.getEditorColor( "caret" );
	mWhitespaceColor = mColorScheme.getEditorColor( "whitespace" );
	mLineBreakColumnColor = mColorScheme.getEditorColor( "line_break_column" );
	mMatchingBracketColor = mColorScheme.getEditorColor( "matching_bracket" );
	mSelectionMatchColor = mColorScheme.getEditorColor( "matching_selection" );
	mMinimapBackgroundColor = mColorScheme.getEditorColor( "minimap_background" );
	mMinimapVisibleAreaColor = mColorScheme.getEditorColor( "minimap_visible_area" );
	mMinimapCurrentLineColor = mColorScheme.getEditorColor( "minimap_current_line" );
	mMinimapHoverColor = mColorScheme.getEditorColor( "minimap_hover" );
	mMinimapHighlightColor = mColorScheme.getEditorColor( "minimap_highlight" );
	mMinimapSelectionColor = mColorScheme.getEditorColor( "minimap_selection" );
}

void UICodeEditor::setColorScheme( const SyntaxColorScheme& colorScheme ) {
	mColorScheme = colorScheme;
	updateColorScheme();
	invalidateDraw();
}

bool UICodeEditor::hasDocument() const {
	return mDoc.get() != nullptr;
}

std::shared_ptr<Doc::TextDocument> UICodeEditor::getDocumentRef() const {
	return mDoc;
}

const TextDocument& UICodeEditor::getDocument() const {
	return *mDoc.get();
}

TextDocument& UICodeEditor::getDocument() {
	return *mDoc.get();
}

void UICodeEditor::setDocument( std::shared_ptr<TextDocument> doc ) {
	if ( mDoc.get() != doc.get() ) {
		mDoc->unregisterClient( this );
		if ( mDoc.use_count() == 1 )
			onDocumentClosed( mDoc.get() );
		mDoc = doc;
		mDoc->registerClient( this );
		invalidateEditor();
		invalidateDraw();
		onDocumentChanged();
	}
}

bool UICodeEditor::isDirty() const {
	return mDoc->isDirty();
}

void UICodeEditor::invalidateEditor( bool dirtyScroll ) {
	mDirtyEditor = true;
	mDirtyScroll = dirtyScroll;
}

void UICodeEditor::invalidateLongestLineWidth() {
	mLongestLineWidthDirty = true;
	mLongestLineWidthLastUpdate.restart();
}

Uint32 UICodeEditor::onFocus() {
	if ( !mLocked ) {
		getUISceneNode()->getWindow()->startTextInput();
		mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();
		resetCursor();
		mDoc->setActiveClient( this );
	}
	for ( auto& plugin : mPlugins )
		plugin->onFocus( this );
	return UIWidget::onFocus();
}

Uint32 UICodeEditor::onFocusLoss() {
	if ( mMouseDown )
		getUISceneNode()->getWindow()->getInput()->captureMouse( false );
	stopMinimapDragging( getUISceneNode()->getWindow()->getInput()->getMousePosf() );
	mMouseDown = false;
	mCursorVisible = false;
	getSceneNode()->getWindow()->stopTextInput();
	getUISceneNode()->setCursor( Cursor::Arrow );
	if ( mDoc->getActiveClient() == this )
		mDoc->setActiveClient( nullptr );
	for ( auto& plugin : mPlugins )
		plugin->onFocusLoss( this );
	return UIWidget::onFocusLoss();
}

Uint32 UICodeEditor::onTextInput( const TextInputEvent& event ) {
	if ( mLocked || NULL == mFont )
		return 0;
	Input* input = getUISceneNode()->getWindow()->getInput();

	if ( ( input->isLeftAltPressed() && !event.getText().empty() && event.getText()[0] == '\t' ) ||
		 ( input->isLeftControlPressed() && !input->isAltGrPressed() ) || input->isMetaPressed() ||
		 input->isLeftAltPressed() )
		return 0;

	if ( mLastExecuteEventId == getUISceneNode()->getWindow()->getInput()->getEventsSentId() )
		return 0;

	mDoc->textInput( event.getText() );

	checkAutoCloseXMLTag( event.getText() );

	for ( auto& plugin : mPlugins )
		if ( plugin->onTextInput( this, event ) )
			return 1;

	return 0;
}

Uint32 UICodeEditor::onKeyDown( const KeyEvent& event ) {
	if ( NULL == mFont || mUISceneNode->getUIEventDispatcher()->justGainedFocus() )
		return 0;

	for ( auto& plugin : mPlugins )
		if ( plugin->onKeyDown( this, event ) )
			return 1;

	std::string cmd = mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( !cmd.empty() ) {
		// Allow copy selection on locked mode
		if ( !mLocked || mUnlockedCmd.find( cmd ) != mUnlockedCmd.end() ) {
			mDoc->execute( cmd );
			mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();
			return 1;
		}
	}
	return 0;
}

Uint32 UICodeEditor::onKeyUp( const KeyEvent& event ) {
	for ( auto& plugin : mPlugins )
		if ( plugin->onKeyUp( this, event ) )
			return 1;
	if ( mHandShown && !getUISceneNode()->getWindow()->getInput()->isControlPressed() )
		resetLinkOver();
	return UIWidget::onKeyUp( event );
}

TextPosition UICodeEditor::resolveScreenPosition( const Vector2f& position, bool clamp ) const {
	Vector2f localPos( convertToNodeSpace( position ) );
	localPos += mScroll;
	localPos.x -= mPaddingPx.Left + getGutterWidth();
	localPos.y -= mPaddingPx.Top;
	localPos.y -= getPluginsTopSpace();
	Int64 line = (Int64)eefloor( localPos.y / getLineHeight() );
	if ( clamp )
		line = eeclamp<Int64>( line, 0, (Int64)( mDoc->linesCount() - 1 ) );
	return TextPosition( line, getColFromXOffset( line, localPos.x ) );
}

Rectf UICodeEditor::getScreenPosition( const TextPosition& position ) const {
	Float lineHeight = getLineHeight();
	Vector2f screenStart( getScreenStart() );
	Vector2f start( screenStart.x + getGutterWidth(), screenStart.y );
	Vector2f startScroll( start - mScroll );
	return { { startScroll.x + getXOffsetColSanitized( position ),
			   startScroll.y + lineHeight * position.line() },
			 { getGlyphWidth(), lineHeight } };
}

const Float& UICodeEditor::getPluginsTopSpace() const {
	return mPluginsTopSpace;
}

Vector2f UICodeEditor::getViewPortLineCount() const {
	return Vector2f(
		eefloor( getViewportWidth() / getGlyphWidth() ),
		eefloor( ( mSize.getHeight() - mPaddingPx.Top - getPluginsTopSpace() -
				   ( mHScrollBar->isVisible() ? mHScrollBar->getPixelsSize().getHeight() : 0.f ) ) /
				 getLineHeight() ) );
}

Sizef UICodeEditor::getMaxScroll() const {
	Vector2f vplc( getViewPortLineCount() );
	return Sizef( eemax( 0.f, mLongestLineWidth - getViewportWidth() ),
				  vplc.y > mDoc->linesCount() - 1
					  ? 0.f
					  : eefloor( mDoc->linesCount() - getViewPortLineCount().y ) *
							getLineHeight() );
}

UIMenuItem* UICodeEditor::menuAdd( UIPopUpMenu* menu, const std::string& translateKey,
								   const String& translateString, const std::string& icon,
								   const std::string& cmd ) {
	UIMenuItem* menuItem =
		menu->add( getTranslatorString( "@string/uicodeeditor_" + translateKey, translateString ),
				   findIcon( icon ), mKeyBindings.getCommandKeybindString( cmd ) );
	menuItem->setId( cmd );
	return menuItem;
}

void UICodeEditor::createDefaultContextMenuOptions( UIPopUpMenu* menu ) {
	if ( !mCreateDefaultContextMenuOptions )
		return;

	menuAdd( menu, "undo", "Undo", "undo", "undo" )->setEnabled( mDoc->hasUndo() );
	menuAdd( menu, "redo", "Redo", "redo", "redo" )->setEnabled( mDoc->hasRedo() );
	menu->addSeparator();

	menuAdd( menu, "cut", "Cut", "cut", "cut" )->setEnabled( mDoc->hasSelection() );
	menuAdd( menu, "copy", "Copy", "copy", "copy" )->setEnabled( mDoc->hasSelection() );
	menuAdd( menu, "paste", "Paste", "paste", "paste" );
	menuAdd( menu, "delete", "Delete", "delete-text", "delete-to-next-char" );
	menu->addSeparator();
	menuAdd( menu, "select_all", "Select All", "select-all", "select-all" );

	if ( mDoc->hasFilepath() ) {
		menu->addSeparator();

		menuAdd( menu, "open_containing_folder", "Open Containing Folder...", "folder-open",
				 "open-containing-folder" );

		menuAdd( menu, "copy_containing_folder_path", "Copy Containing Folder Path...", "copy",
				 "copy-containing-folder-path" );

		menuAdd( menu, "copy_file_path", "Copy File Path", "copy", "copy-file-path" );
	}
}

bool UICodeEditor::createContextMenu() {
	Rectf pos( getScreenPosition( mDoc->getSelection().start() ) );
	return onCreateContextMenu( pos.getPosition().asInt(), 0 );
}

bool UICodeEditor::onCreateContextMenu( const Vector2i& position, const Uint32& flags ) {
	if ( mCurrentMenu )
		return false;

	UIPopUpMenu* menu = UIPopUpMenu::New();

	createDefaultContextMenuOptions( menu );

	ContextMenuEvent event( this, menu, Event::OnCreateContextMenu, position, flags );
	sendEvent( &event );

	for ( auto& plugin : mPlugins )
		if ( plugin->onCreateContextMenu( this, menu, position, flags ) )
			return false;

	if ( menu->getCount() == 0 ) {
		menu->close();
		return false;
	}

	menu->setCloseOnHide( true );
	menu->addEventListener( Event::OnItemClicked, [&, menu]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string txt( item->getId() );
		mDoc.get()->execute( txt );
		menu->hide();
	} );

	Vector2f pos( position.asFloat() );
	runOnMainThread( [this, menu, pos]() {
		Vector2f npos( pos );
		menu->nodeToWorldTranslation( npos );
		UIMenu::findBestMenuPos( npos, menu );
		menu->setPixelsPosition( npos );
		menu->show();
		mCurrentMenu = menu;
	} );
	menu->addEventListener( Event::OnMenuHide, [&]( const Event* ) {
		if ( !isClosing() )
			setFocus();
	} );
	menu->addEventListener( Event::OnClose, [&]( const Event* ) { mCurrentMenu = nullptr; } );
	return true;
}

Int64 UICodeEditor::calculateMinimapClickedLine( const Vector2i& position ) {
	auto lineRange = getVisibleLineRange();
	Vector2f start( getScreenStart() );
	Float lineSpacing = getMinimapLineSpacing();
	Rectf rect( getMinimapRect( start ) );
	Int64 visibleLinesCount = ( lineRange.second - lineRange.first );
	Int64 visibleLinesStart = lineRange.first;
	Float scrollerHeight = visibleLinesCount * lineSpacing;
	size_t lineCount = mDoc->linesCount();
	Int64 maxMinmapLines = eefloor( rect.getHeight() / lineSpacing );
	Int64 minimapStartLine = 0;
	if ( isMinimapFileTooLarge() ) {
		Float scrollPos = ( visibleLinesStart - 1 ) / (Float)( lineCount - visibleLinesCount - 1 );
		scrollPos = eeclamp( scrollPos, 0.f, 1.f );
		Float scrollPosPixels = scrollPos * ( rect.getHeight() - scrollerHeight );
		minimapStartLine = visibleLinesStart - eefloor( scrollPosPixels / lineSpacing );
		minimapStartLine =
			eemax( (Int64)0, eemin( minimapStartLine, (Int64)lineCount - maxMinmapLines ) );
	}
	Float dy = position.y - rect.Top;
	Int64 ret = minimapStartLine + eefloor( dy / lineSpacing );
	return eeclamp( ret, (Int64)0, (Int64)lineCount );
}

Uint32 UICodeEditor::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseDown( this, position, flags ) )
			return UIWidget::onMouseDown( position, flags );

	if ( mMinimapEnabled ) {
		updateMipmapHover( position.asFloat() );

		Rectf rect( getMinimapRect( getScreenStart() ) );
		if ( ( flags & EE_BUTTON_LMASK ) && !getEventDispatcher()->isNodeDragging() &&
			 getEventDispatcher()->getMouseDownNode() == this && !mMinimapDragging &&
			 rect.contains( position.asFloat() ) ) {
			if ( mMouseDown )
				return 1;
			if ( !mMinimapHover && !mMouseDown ) {
				mMinimapScrollOffset = 0;
				scrollTo( { calculateMinimapClickedLine( position ), 0 }, true, true, false );
				return 1;
			}
			mMouseDown = true;
			mMinimapScrollOffset =
				calculateMinimapClickedLine( position ) - getVisibleLineRange().first;
			mMinimapDragging = true;
			getEventDispatcher()->setNodeDragging( this );
			mVScrollBar->setEnabled( false );
			getUISceneNode()->setCursor( Cursor::Arrow );
			return 1;
		} else if ( mMinimapDragging ) {
			if ( ( flags & EE_BUTTON_LMASK ) && mMouseDown &&
				 rect.contains( position.asFloat() ) ) {
				scrollTo( { calculateMinimapClickedLine( position ) - mMinimapScrollOffset, 0 },
						  false, true, false );
				getUISceneNode()->setCursor( Cursor::Arrow );
			}
			return 1;
		}
	}

	if ( ( flags & ( EE_BUTTON_LMASK | EE_BUTTON_RMASK ) ) && isTextSelectionEnabled() &&
		 !getEventDispatcher()->isNodeDragging() && NULL != mFont && !mMouseDown &&
		 getEventDispatcher()->getMouseDownNode() == this ) {
		mMouseDown = true;
		Input* input = getUISceneNode()->getWindow()->getInput();
		input->captureMouse( true );
		setFocus();
		if ( flags & EE_BUTTON_LMASK ) {
			if ( input->isModState( KEYMOD_LALT | KEYMOD_SHIFT ) ) {
				TextRange range( mDoc->getSelection().start(),
								 resolveScreenPosition( position.asFloat() ) );
				range.normalize();
				range = mDoc->sanitizeRange( range );
				for ( Int64 i = range.start().line(); i < range.end().line(); ++i )
					mDoc->addSelection( { i, range.start().column() } );
			} else if ( input->isModState( KEYMOD_SHIFT ) ) {
				mDoc->selectTo( resolveScreenPosition( position.asFloat() ) );
			} else if ( input->isModState( KEYMOD_CTRL ) &&
						checkMouseOverLink( position ).empty() ) {
				TextPosition pos( resolveScreenPosition( position.asFloat() ) );
				if ( !mDoc->selectionExists( pos ) )
					mDoc->addSelection( { pos, pos } );
			} else {
				mDoc->setSelection( resolveScreenPosition( position.asFloat() ) );
			}
		} else if ( !mDoc->hasSelection() ) {
			mDoc->setSelection( resolveScreenPosition( position.asFloat() ) );
		}
	}
	return UIWidget::onMouseDown( position, flags );
}

void UICodeEditor::updateMipmapHover( const Vector2f& position ) {
	Rectf rect( getMinimapRect( getScreenStart() ) );
	if ( !rect.contains( position ) && !mMinimapHover )
		return;

	Float lineSpacing = getMinimapLineSpacing();
	auto lineRange = getVisibleLineRange();
	int visibleLinesCount = ( lineRange.second - lineRange.first );
	int visibleLinesStart = lineRange.first;
	Float scrollerHeight = visibleLinesCount * lineSpacing;
	int lineCount = mDoc->linesCount();
	Float visibleY = rect.Top + visibleLinesStart * lineSpacing;

	if ( isMinimapFileTooLarge() ) {
		Float scrollPos = ( visibleLinesStart - 1 ) / (Float)( lineCount - visibleLinesCount - 1 );
		scrollPos = eeclamp( scrollPos, 0.f, 1.f );
		Float scrollPosPixels = scrollPos * ( rect.getHeight() - scrollerHeight );
		visibleY = rect.Top + scrollPosPixels;
		Float t = ( lineCount - visibleLinesStart ) / visibleLinesCount;
		if ( t <= 1 )
			visibleY += scrollerHeight * ( 1.f - t );
	}

	Rectf rectHover( { { rect.Left, visibleY }, Sizef( rect.getWidth(), scrollerHeight ) } );
	bool prevState = mMinimapHover;
	mMinimapHover = rectHover.contains( position.asFloat() );

	if ( mMinimapHover != prevState )
		invalidateDraw();
}

Uint32 UICodeEditor::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseMove( this, position, flags ) )
			return UIWidget::onMouseMove( position, flags );

	bool minimapHover = false;
	if ( mMinimapEnabled ) {
		updateMipmapHover( position.asFloat() );
		if ( mMinimapDragging && ( flags & EE_BUTTON_LMASK ) ) {
			scrollTo( { calculateMinimapClickedLine( position ) - mMinimapScrollOffset, 0 }, false,
					  true, false );
			getUISceneNode()->setCursor( Cursor::Arrow );
			return 1;
		}
		minimapHover = getMinimapRect( getScreenStart() ).contains( position.asFloat() );
		if ( ( flags & EE_BUTTON_LMASK ) && minimapHover )
			return 1;
	}

	if ( !minimapHover && isTextSelectionEnabled() &&
		 !getUISceneNode()->getEventDispatcher()->isNodeDragging() && NULL != mFont && mMouseDown &&
		 ( flags & EE_BUTTON_LMASK ) ) {
		TextRange selection = mDoc->getSelection();
		selection.setStart( resolveScreenPosition( position.asFloat() ) );
		mDoc->setSelection( selection );
	}

	if ( minimapHover ) {
		getUISceneNode()->setCursor( Cursor::Arrow );
	} else {
		checkMouseOverColor( position );

		checkMouseOverLink( position );
	}

	return UIWidget::onMouseMove( position, flags );
}

Uint32 UICodeEditor::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseUp( this, position, flags ) )
			return UIWidget::onMouseUp( position, flags );

	if ( NULL == mFont )
		return UIWidget::onMouseUp( position, flags );

	bool minimapHover =
		mMinimapEnabled && getMinimapRect( getScreenStart() ).contains( position.asFloat() );

	if ( flags & EE_BUTTON_LMASK ) {
		stopMinimapDragging( position.asFloat() );

		if ( mMouseDown ) {
			mMouseDown = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		}
	}

	if ( flags & EE_BUTTON_WDMASK ) {
		if ( getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
			mDoc->execute( "font-size-shrink" );
		} else {
			setScrollY( mScroll.y + PixelDensity::dpToPx( mMouseWheelScroll ) );
		}
		invalidateDraw();
	} else if ( flags & EE_BUTTON_WUMASK ) {
		if ( getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
			mDoc->execute( "font-size-grow" );
		} else {
			setScrollY( mScroll.y - PixelDensity::dpToPx( mMouseWheelScroll ) );
		}
		invalidateDraw();
	} else if ( flags & EE_BUTTON_WRMASK ) {
		setScrollX( mScroll.x + PixelDensity::dpToPx( mMouseWheelScroll ) );
	} else if ( flags & EE_BUTTON_WLMASK ) {
		setScrollX( mScroll.x - PixelDensity::dpToPx( mMouseWheelScroll ) );
	} else if ( !minimapHover && ( flags & EE_BUTTON_RMASK ) ) {
		onCreateContextMenu( position, flags );
	}

	return UIWidget::onMouseUp( position, flags );
}

Uint32 UICodeEditor::onMouseClick( const Vector2i& position, const Uint32& flags ) {
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseClick( this, position, flags ) )
			return UIWidget::onMouseClick( position, flags );

	if ( mMinimapEnabled ) {
		Rectf rect( getMinimapRect( getScreenStart() ) );
		if ( ( flags & EE_BUTTON_LMASK ) && rect.contains( position.asFloat() ) )
			return 1;
	}

	if ( ( flags & EE_BUTTON_LMASK ) &&
		 getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
		String link( checkMouseOverLink( position ) );
		if ( !link.empty() ) {
			Engine::instance()->openURI( link.toUtf8() );
			resetLinkOver();
		}
	} else if ( ( flags & EE_BUTTON_LMASK ) &&
				mLastDoubleClick.getElapsedTime() < Milliseconds( 300.f ) ) {
		mDoc->selectLine();
	} else if ( ( flags & EE_BUTTON_MMASK ) && isMouseOverMeOrChilds() ) {
		auto txt( getUISceneNode()->getWindow()->getClipboard()->getText() );
		if ( !isLocked() && !txt.empty() ) {
			if ( mDoc->hasSelection() ) {
				auto selTxt = mDoc->getSelectedText();
				if ( !selTxt.empty() )
					txt = selTxt;
			}
			mDoc->setSelection( resolveScreenPosition( position.asFloat() ) );
			mDoc->textInput( txt );
		}
	}
	return 1;
}

Uint32 UICodeEditor::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseDoubleClick( this, position, flags ) )
			return UIWidget::onMouseDoubleClick( position, flags );

	if ( mLocked || NULL == mFont )
		return 1;

	if ( mMinimapEnabled ) {
		Rectf rect( getMinimapRect( getScreenStart() ) );
		if ( ( flags & EE_BUTTON_LMASK ) && rect.contains( position.asFloat() ) )
			return 1;
	}

	if ( flags & EE_BUTTON_LMASK ) {
		mDoc->selectWord( false );
		mLastDoubleClick.restart();
		checkColorPickerAction();
	}
	return 1;
}

Uint32 UICodeEditor::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseOver( this, position, flags ) )
			return UIWidget::onMouseOver( position, flags );
	if ( getEventDispatcher()->getMouseOverNode() == this )
		getUISceneNode()->setCursor( !mLocked ? Cursor::IBeam : Cursor::Arrow );
	return UIWidget::onMouseOver( position, flags );
}

Uint32 UICodeEditor::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	if ( mMinimapHover ) {
		mMinimapHover = false;
		invalidateDraw();
	}
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseLeave( this, position, flags ) )
			return UIWidget::onMouseLeave( position, flags );
	getUISceneNode()->setCursor( Cursor::Arrow );
	return UIWidget::onMouseLeave( position, flags );
}

void UICodeEditor::checkColorPickerAction() {
	if ( !mEnableColorPickerOnSelection )
		return;
	String text( mDoc->getSelectedText() );
	TextRange range( mDoc->getSelection( true ) );
	if ( range.start().line() != range.end().line() )
		return;
	const String& line = mDoc->line( range.end().line() ).getText();
	bool isHash = range.start().column() > 0 &&
				  mDoc->line( range.start().line() ).getText()[range.start().column() - 1] == '#' &&
				  ( text.size() == 6 || text.size() == 8 ) && String::isHexNotation( text );
	bool isRgba = !isHash && text == "rgba" && range.end().column() < (Int64)line.size() - 1 &&
				  line[range.end().column()] == '(';
	bool isRgb = !isHash && !isRgba && text == "rgb" &&
				 range.end().column() < (Int64)line.size() - 1 && line[range.end().column()] == '(';
	if ( isHash || isRgb || isRgba ) {
		UIColorPicker* colorPicker = NULL;
		if ( isHash ) {
			colorPicker = UIColorPicker::NewModal( this, [&]( Color color ) {
				mDoc->replaceSelection( color.toHexString( false ) );
			} );
			colorPicker->setColor( Color( '#' + text ) );
		} else if ( isRgba || isRgb ) {
			TextPosition position = mDoc->getMatchingBracket(
				{ range.start().line(), static_cast<Int64>( range.end().column() ) }, '(', ')', 1 );
			if ( position.isValid() ) {
				mDoc->setSelection( { position.line(), position.column() + 1 }, range.start() );
				colorPicker = UIColorPicker::NewModal( this, [&, isRgba]( Color color ) {
					mDoc->replaceSelection( isRgba || color.a != 255 ? color.toRgbaString()
																	 : color.toRgbString() );
				} );
				colorPicker->setColor( Color::fromString( mDoc->getSelectedText() ) );
			}
		}
		if ( colorPicker )
			colorPicker->getUIWindow()->addEventListener(
				Event::OnWindowClose, [&]( const Event* ) {
					if ( !SceneManager::instance()->isShootingDown() )
						setFocus();
				} );
	}
}

Vector2f UICodeEditor::getRelativeScreenPosition( const TextPosition& pos ) {
	Float gutterWidth = getGutterWidth();
	Vector2f start( gutterWidth, getPluginsTopSpace() );
	Vector2f startScroll( start - mScroll );
	auto lineHeight = getLineHeight();
	return { startScroll.x + getXOffsetCol( pos ),
			 startScroll.y + pos.line() * lineHeight + getLineOffset() };
}

bool UICodeEditor::getShowLinesRelativePosition() const {
	return mShowLinesRelativePosition;
}

void UICodeEditor::showLinesRelativePosition( bool showLinesRelativePosition ) {
	mShowLinesRelativePosition = showLinesRelativePosition;
}

void UICodeEditor::drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							   const TextPosition& cursor ) {
	if ( mCursorVisible && !mLocked && isTextSelectionEnabled() ) {
		Vector2f cursorPos( startScroll.x + getXOffsetCol( cursor ),
							startScroll.y + cursor.line() * lineHeight + getLineOffset() );
		Primitives primitives;
		primitives.setColor( Color( mCaretColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 2 ), getFontHeight() ) ) );
	}
}

void UICodeEditor::onSizeChange() {
	UIWidget::onSizeChange();
	invalidateEditor( false );
}

void UICodeEditor::onPaddingChange() {
	UIWidget::onPaddingChange();
	invalidateEditor( false );
}

void UICodeEditor::findLongestLine() {
	if ( mHorizontalScrollBarEnabled ) {
		mLongestLineWidth = 0;
		for ( size_t lineIndex = 0; lineIndex < mDoc->linesCount(); lineIndex++ ) {
			mLongestLineWidth = eemax( mLongestLineWidth, getLineWidth( lineIndex ) );
		}
	}
}

Float UICodeEditor::getLineWidth( const Int64& lineIndex ) {
	if ( mFont && !mFont->isMonospace() )
		return getLineText( lineIndex ).getTextWidth() + getGlyphWidth();
	return getTextWidth( mDoc->line( lineIndex ).getText() );
}

void UICodeEditor::updateScrollBar() {
	int notVisibleLineCount = (int)mDoc->linesCount() - (int)getViewPortLineCount().y;

	if ( mLongestLineWidthDirty && mFont && mFont->isMonospace() ) {
		updateLongestLineWidth();
	}

	mHScrollBar->setEnabled( false );
	mHScrollBar->setVisible( false );

	mVScrollBar->setPixelsSize( mVScrollBar->getPixelsSize().getWidth(), mSize.getHeight() );

	if ( mHorizontalScrollBarEnabled ) {
		mHScrollBar->setPixelsPosition( 0, mSize.getHeight() -
											   mHScrollBar->getPixelsSize().getHeight() );
		mHScrollBar->setPixelsSize( mSize.getWidth() -
										( mVerticalScrollBarEnabled && notVisibleLineCount > 0
											  ? mVScrollBar->getPixelsSize().getWidth()
											  : 0 ),
									mHScrollBar->getPixelsSize().getHeight() );
		Float viewPortWidth = getViewportWidth();
		mHScrollBar->setPageStep( viewPortWidth / mLongestLineWidth );
		mHScrollBar->setClickStep( 0.2f );
		mHScrollBar->setEnabled( mLongestLineWidth > viewPortWidth );
		mHScrollBar->setVisible( mLongestLineWidth > viewPortWidth );
	}

	mVScrollBar->setPixelsPosition( mSize.getWidth() - mVScrollBar->getPixelsSize().getWidth(), 0 );
	mVScrollBar->setPageStep( getViewPortLineCount().y / (float)mDoc->linesCount() );
	mVScrollBar->setClickStep( 0.2f );
	mVScrollBar->setEnabled( mVerticalScrollBarEnabled && notVisibleLineCount > 0 );
	mVScrollBar->setVisible( mVerticalScrollBarEnabled && notVisibleLineCount > 0 );
	setScrollY( mScroll.y );
}

void UICodeEditor::goToLine( const TextPosition& position, bool centered, bool forceExactPosition,
							 bool scrollX ) {
	mDoc->setSelection( position );
	scrollTo( mDoc->getSelection().start(), centered, forceExactPosition, scrollX );
}

bool UICodeEditor::getAutoCloseBrackets() const {
	return mDoc->getAutoCloseBrackets();
}

void UICodeEditor::setAutoCloseBrackets( bool autoCloseBrackets ) {
	mDoc->setAutoCloseBrackets( autoCloseBrackets );
}

bool UICodeEditor::getInteractiveLinks() const {
	return mInteractiveLinks;
}

void UICodeEditor::setInteractiveLinks( bool newInteractiveLinks ) {
	mInteractiveLinks = newInteractiveLinks;
}

UILoader* UICodeEditor::getLoader() {
	if ( nullptr == mLoader )
		mLoader = UILoader::New();
	return mLoader;
}

bool UICodeEditor::getDisplayLoaderIfDocumentLoading() const {
	return mDisplayLoaderIfDocumentLoading;
}

void UICodeEditor::setDisplayLoaderIfDocumentLoading( bool newDisplayLoaderIfDocumentLoading ) {
	mDisplayLoaderIfDocumentLoading = newDisplayLoaderIfDocumentLoading;
	if ( !mDisplayLoaderIfDocumentLoading && mLoader != nullptr && mLoader->isVisible() ) {
		mLoader->setVisible( false );
		mLoader->close();
		mLoader = nullptr;
	}
}

size_t UICodeEditor::getMenuIconSize() const {
	return mMenuIconSize;
}

void UICodeEditor::setMenuIconSize( size_t menuIconSize ) {
	mMenuIconSize = menuIconSize;
}

bool UICodeEditor::getCreateDefaultContextMenuOptions() const {
	return mCreateDefaultContextMenuOptions;
}

void UICodeEditor::setCreateDefaultContextMenuOptions( bool createDefaultContextMenuOptions ) {
	mCreateDefaultContextMenuOptions = createDefaultContextMenuOptions;
}

void UICodeEditor::openContainingFolder() {
	Engine::instance()->openURI( mDoc->getFileInfo().getDirectoryPath() );
}

void UICodeEditor::copyContainingFolderPath() {
	getUISceneNode()->getWindow()->getClipboard()->setText(
		mDoc->getFileInfo().getDirectoryPath() );
}

void UICodeEditor::copyFilePath() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc->getFilePath() );
}

void UICodeEditor::scrollToCursor( bool centered ) {
	scrollTo( mDoc->getSelection().start(), centered );
}

void UICodeEditor::updateEditor() {
	mDoc->setPageSize( getVisibleLinesCount() );
	if ( mDirtyScroll && mDoc->getActiveClient() == this )
		scrollTo( mDoc->getSelection().start() );
	updateScrollBar();
	mDirtyEditor = false;
	mDirtyScroll = false;
}

void UICodeEditor::onDocumentTextChanged( const DocumentContentChange& ) {
	invalidateDraw();
	checkMatchingBrackets();
	sendCommonEvent( Event::OnTextChanged );
	invalidateLongestLineWidth();
}

void UICodeEditor::onDocumentCursorChange( const Doc::TextPosition& ) {
	resetCursor();
	checkMatchingBrackets();
	invalidateEditor();
	invalidateDraw();
	onCursorPosChange();
}

void UICodeEditor::onDocumentSelectionChange( const Doc::TextRange& ) {
	resetCursor();
	invalidateDraw();
	sendCommonEvent( Event::OnSelectionChanged );
}

void UICodeEditor::onDocumentLineCountChange( const size_t&, const size_t& ) {
	updateScrollBar();
}

void UICodeEditor::onDocumentLineChanged( const Int64& lineNumber ) {
	mDoc->getHighlighter()->invalidate( lineNumber );
	if ( mFont && !mFont->isMonospace() )
		updateLineCache( lineNumber );

	if ( !mHighlightWord.isEmpty() )
		updateHighlightWordCache();
}

void UICodeEditor::onDocumentUndoRedo( const TextDocument::UndoRedo& ) {
	onDocumentSelectionChange( {} );
	DocEvent event( this, mDoc.get(), Event::OnDocumentUndoRedo );
	sendEvent( &event );
}

void UICodeEditor::onDocumentSaved( TextDocument* doc ) {
	DocEvent event( this, doc, Event::OnDocumentSave );
	sendEvent( &event );
}

void UICodeEditor::onDocumentMoved( TextDocument* doc ) {
	DocEvent event( this, doc, Event::OnDocumentMoved );
	sendEvent( &event );
}

void UICodeEditor::onDocumentClosed( TextDocument* doc ) {
	DocEvent event( this, doc, Event::OnDocumentClosed );
	sendEvent( &event );
}

void UICodeEditor::onDocumentDirtyOnFileSystem( TextDocument* doc ) {
	DocEvent event( this, doc, Event::OnDocumentDirtyOnFileSysten );
	sendEvent( &event );
}

std::pair<Uint64, Uint64> UICodeEditor::getVisibleLineRange() const {
	Float lineHeight = getLineHeight();
	Float minLine = eemax( 0.f, eefloor( mScroll.y / lineHeight ) );
	Float maxLine = eemin( mDoc->linesCount() - 1.f,
						   eefloor( ( mSize.getHeight() + mScroll.y ) / lineHeight ) + 1 );
	return std::make_pair<Uint64, Uint64>( (Uint64)minLine, (Uint64)maxLine );
}

bool UICodeEditor::isLineVisible( const Uint64& line ) const {
	auto range = getVisibleLineRange();
	return line >= range.first && line <= range.second;
}

int UICodeEditor::getVisibleLinesCount() const {
	auto lines = getVisibleLineRange();
	return lines.second - lines.first;
}

const StyleSheetLength& UICodeEditor::getLineSpacing() const {
	return mLineSpacing;
}

void UICodeEditor::setLineSpacing( const StyleSheetLength& lineSpace ) {
	if ( lineSpace != mLineSpacing ) {
		mLineSpacing = lineSpace;
		invalidateDraw();
	}
}

void UICodeEditor::scrollTo( const TextPosition& position, bool centered, bool forceExactPosition,
							 bool scrollX ) {
	auto lineRange = getVisibleLineRange();

	Int64 minDistance = mHScrollBar->isVisible() ? 3 : 2;

	if ( forceExactPosition || position.line() <= (Int64)lineRange.first ||
		 position.line() >= (Int64)lineRange.second - minDistance ) {
		// Vertical Scroll
		Float lineHeight = getLineHeight();
		Float min = eefloor( lineHeight * ( eemax<Float>( 0, position.line() - 1 ) ) );
		Float max = eefloor( lineHeight * ( position.line() + minDistance ) - mSize.getHeight() );
		Float halfScreenLines = eefloor( mSize.getHeight() / lineHeight * 0.5f );

		if ( forceExactPosition ) {
			setScrollY(
				lineHeight *
				( eemax<Float>( 0, position.line() - 1 - ( centered ? halfScreenLines : 0 ) ) ) );
		} else if ( min < mScroll.y ) {
			if ( centered ) {
				if ( position.line() - 1 - halfScreenLines >= 0 )
					min = eefloor( lineHeight *
								   ( eemax<Float>( 0, position.line() - 1 - halfScreenLines ) ) );
			}
			setScrollY( min );
		} else if ( max > mScroll.y ) {
			if ( centered ) {
				max = eefloor( lineHeight * ( position.line() + minDistance + halfScreenLines ) -
							   mSize.getHeight() );
				max = eemin( max, getMaxScroll().y );
			}
			setScrollY( max );
		}
	}

	// Horizontal Scroll
	if ( !scrollX )
		return;
	Float offsetX = getXOffsetCol( position );
	Float glyphSize = getGlyphWidth();
	Float minVisibility = glyphSize;
	Float viewPortWidth = getViewportWidth();
	if ( offsetX + minVisibility > mScroll.x + viewPortWidth ) {
		setScrollX( eefloor( eemax( 0.f, offsetX + minVisibility - viewPortWidth ) ) );
	} else if ( offsetX < mScroll.x ) {
		setScrollX( eefloor( eemax( 0.f, offsetX - minVisibility ) ) );
	}
}

const UICodeEditor::MinimapConfig& UICodeEditor::getMinimapConfig() const {
	return mMinimapConfig;
}

void UICodeEditor::setMinimapConfig( const UICodeEditor::MinimapConfig& minimapConfig ) {
	mMinimapConfig = minimapConfig;
}

bool UICodeEditor::isMinimapShown() const {
	return mMinimapEnabled;
}

void UICodeEditor::showMinimap( bool showMinimap ) {
	mMinimapEnabled = showMinimap;
}

void UICodeEditor::setScrollX( const Float& val, bool emmitEvent ) {
	Float oldVal = mScroll.x;
	mScroll.x = eefloor( eeclamp<Float>( val, 0.f, getMaxScroll().x ) );
	if ( oldVal != mScroll.x ) {
		invalidateDraw();
		if ( mHorizontalScrollBarEnabled && emmitEvent )
			mHScrollBar->setValue( mScroll.x / getMaxScroll().x, false );
	}
}

void UICodeEditor::setScrollY( const Float& val, bool emmitEvent ) {
	Float oldVal = mScroll.y;
	mScroll.y = eefloor( eeclamp<Float>( val, 0, getMaxScroll().y ) );
	if ( oldVal != mScroll.y ) {
		invalidateDraw();
		if ( mVerticalScrollBarEnabled && emmitEvent )
			mVScrollBar->setValue( mScroll.y / getMaxScroll().y, false );
	}
}

Float UICodeEditor::getXOffsetCol( const TextPosition& position ) const {
	if ( mFont && !mFont->isMonospace() ) {
		return getLineText( position.line() )
			.findCharacterPos(
				( position.column() == (Int64)mDoc->line( position.line() ).getText().size() )
					? position.column() - 1
					: position.column() )
			.x;
	}

	const String& line = mDoc->line( position.line() ).getText();
	Float glyphWidth = getGlyphWidth();
	Float x = 0;
	for ( auto i = 0; i < position.column(); i++ ) {
		if ( line[i] == '\t' ) {
			x += glyphWidth * mTabWidth;
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			x += glyphWidth;
		}
	}
	return x;
}

size_t UICodeEditor::characterWidth( const String& str ) const {
	Int64 cc = str.size();
	Int64 count = 0;
	for ( Int64 i = 0; i < cc; i++ )
		count += str[i] == '\t' ? mTabWidth : 1;
	return count;
}

Float UICodeEditor::getTextWidth( const String& line ) const {
	if ( mFont && !mFont->isMonospace() ) {
		Float fontSize = PixelDensity::pxToDp( getCharacterSize() );
		Text txt( "", mFont, fontSize );
		txt.setTabWidth( mTabWidth );
		txt.setStyleConfig( mFontStyleConfig );
		txt.setString( line );
		return txt.getTextWidth();
	}

	Float glyphWidth = getGlyphWidth();
	size_t len = line.length();
	Float x = 0;
	for ( size_t i = 0; i < len; i++ )
		x += ( line[i] == '\t' ) ? glyphWidth * mTabWidth : glyphWidth;
	return x;
}

Float UICodeEditor::getXOffsetColSanitized( TextPosition position ) const {
	position.setLine( eeclamp<Int64>( position.line(), 0L, mDoc->linesCount() - 1 ) );
	// This is different from sanitizePosition, sinze allows the last character.
	position.setColumn( eeclamp<Int64>( position.column(), 0L,
										eemax<Int64>( 0, mDoc->line( position.line() ).size() ) ) );
	return getXOffsetCol( position );
}

const bool& UICodeEditor::isLocked() const {
	return mLocked;
}

void UICodeEditor::setLocked( bool locked ) {
	if ( mLocked != locked ) {
		mLocked = locked;
		if ( !mLocked && hasFocus() )
			mDoc->setActiveClient( this );
		invalidateDraw();
	}
}

const Color& UICodeEditor::getLineNumberFontColor() const {
	return mLineNumberFontColor;
}

void UICodeEditor::setLineNumberFontColor( const Color& lineNumberFontColor ) {
	if ( lineNumberFontColor != mLineNumberFontColor ) {
		mLineNumberFontColor = lineNumberFontColor;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getLineNumberActiveFontColor() const {
	return mLineNumberActiveFontColor;
}

void UICodeEditor::setLineNumberActiveFontColor( const Color& lineNumberActiveFontColor ) {
	if ( mLineNumberActiveFontColor != lineNumberActiveFontColor ) {
		mLineNumberActiveFontColor = lineNumberActiveFontColor;
		invalidateDraw();
	}
}

KeyBindings& UICodeEditor::getKeyBindings() {
	return mKeyBindings;
}

void UICodeEditor::setKeyBindings( const KeyBindings& keyBindings ) {
	mKeyBindings = keyBindings;
}

void UICodeEditor::addKeyBindingString( const std::string& shortcut, const std::string& command,
										const bool& allowLocked ) {
	mKeyBindings.addKeybindString( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command,
								  const bool& allowLocked ) {
	mKeyBindings.addKeybind( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::replaceKeyBindingString( const std::string& shortcut, const std::string& command,
											const bool& allowLocked ) {
	mKeyBindings.replaceKeybindString( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::replaceKeyBinding( const KeyBindings::Shortcut& shortcut,
									  const std::string& command, const bool& allowLocked ) {
	mKeyBindings.replaceKeybind( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::addKeyBindsString( const std::map<std::string, std::string>& binds,
									  const bool& allowLocked ) {
	mKeyBindings.addKeybindsString( binds );
	for ( const auto& bind : binds ) {
		if ( allowLocked ) {
			mUnlockedCmd.insert( bind.second );
		}
	}
}

void UICodeEditor::addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds,
								const bool& allowLocked ) {
	mKeyBindings.addKeybinds( binds );
	for ( const auto& bind : binds ) {
		if ( allowLocked ) {
			mUnlockedCmd.insert( bind.second );
		}
	}
}

const bool& UICodeEditor::getHighlightCurrentLine() const {
	return mHighlightCurrentLine;
}

void UICodeEditor::setHighlightCurrentLine( const bool& highlightCurrentLine ) {
	if ( mHighlightCurrentLine != highlightCurrentLine ) {
		mHighlightCurrentLine = highlightCurrentLine;
		invalidateDraw();
	}
}

const Uint32& UICodeEditor::getLineBreakingColumn() const {
	return mLineBreakingColumn;
}

void UICodeEditor::setLineBreakingColumn( const Uint32& lineBreakingColumn ) {
	if ( lineBreakingColumn != mLineBreakingColumn ) {
		mLineBreakingColumn = lineBreakingColumn;
		invalidateDraw();
	}
}

void UICodeEditor::addUnlockedCommand( const std::string& command ) {
	mUnlockedCmd.insert( command );
}

void UICodeEditor::addUnlockedCommands( const std::vector<std::string>& commands ) {
	mUnlockedCmd.insert( commands.begin(), commands.end() );
}

bool UICodeEditor::isUnlockedCommand( const std::string& command ) {
	return mUnlockedCmd.find( command ) != mUnlockedCmd.end();
}

UICodeEditor* UICodeEditor::setFontShadowColor( const Color& color ) {
	if ( mFontStyleConfig.ShadowColor != color ) {
		mFontStyleConfig.ShadowColor = color;
		if ( mFontStyleConfig.ShadowColor != Color::Transparent )
			mFontStyleConfig.Style |= Text::Shadow;
		else
			mFontStyleConfig.Style &= ~Text::Shadow;
		invalidateDraw();
		onFontStyleChanged();
	}

	return this;
}

const Color& UICodeEditor::getFontShadowColor() const {
	return mFontStyleConfig.getFontShadowColor();
}

UICodeEditor* UICodeEditor::setFontShadowOffset( const Vector2f& offset ) {
	if ( mFontStyleConfig.ShadowOffset != offset ) {
		mFontStyleConfig.ShadowOffset = offset;
		invalidateDraw();
		onFontStyleChanged();
	}

	return this;
}

const Vector2f& UICodeEditor::getFontShadowOffset() const {
	return mFontStyleConfig.getFontShadowOffset();
}

void UICodeEditor::setScroll( const Vector2f& val, bool emmitEvent ) {
	setScrollX( val.x, emmitEvent );
	setScrollY( val.y, emmitEvent );
}

bool UICodeEditor::getShowLineEndings() const {
	return mShowLineEndings;
}

void UICodeEditor::setShowLineEndings( bool showLineEndings ) {
	if ( mShowLineEndings != showLineEndings ) {
		mShowLineEndings = showLineEndings;
		invalidateDraw();
	}
}

UICodeEditor* UICodeEditor::setFontStyle( const Uint32& fontStyle ) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mFontStyleConfig.Style = fontStyle;
		invalidateDraw();
		onFontStyleChanged();
	}

	return this;
}

const Uint32& UICodeEditor::getFontStyle() const {
	return mFontStyleConfig.getFontStyle();
}

const Float& UICodeEditor::getOutlineThickness() const {
	return mFontStyleConfig.OutlineThickness;
}

UICodeEditor* UICodeEditor::setOutlineThickness( const Float& outlineThickness ) {
	if ( mFontStyleConfig.OutlineThickness != outlineThickness ) {
		mFontStyleConfig.OutlineThickness = outlineThickness;
		invalidateDraw();
		onFontStyleChanged();
	}

	return this;
}

const Color& UICodeEditor::getOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

UICodeEditor* UICodeEditor::setOutlineColor( const Color& outlineColor ) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mFontStyleConfig.OutlineColor = outlineColor;
		invalidateDraw();
		onFontStyleChanged();
	}

	return this;
}

bool UICodeEditor::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

void UICodeEditor::setTextSelection( const bool& active ) {
	if ( active ) {
		mFlags |= UI_TEXT_SELECTION_ENABLED;
	} else {
		mFlags &= ~UI_TEXT_SELECTION_ENABLED;
	}
}

bool UICodeEditor::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Locked:
			setLocked( attribute.asBool() );
			break;
		case PropertyId::Color:
			setFontColor( attribute.asColor() );
			break;
		case PropertyId::TextShadowColor: {
			setFontShadowColor( attribute.asColor() );
			break;
		}
		case PropertyId::TextShadowOffset:
			setFontShadowOffset( attribute.asVector2f() );
			break;
		case PropertyId::SelectionColor:
			setFontSelectedColor( attribute.asColor() );
			break;
		case PropertyId::SelectionBackColor:
			setFontSelectionBackColor( attribute.asColor() );
			break;
		case PropertyId::FontFamily: {
			Font* font = FontManager::instance()->getByName( attribute.asString() );
			if ( NULL != font && font->loaded() ) {
				setFont( font );
			}
			break;
		}
		case PropertyId::FontSize:
			setFontSize( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::FontStyle: {
			setFontStyle( attribute.asFontStyle() );
			break;
		}
		case PropertyId::TextStrokeWidth:
			setOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			setOutlineColor( attribute.asColor() );
			break;
		case PropertyId::TextSelection:
			setTextSelection( attribute.asBool() );
			break;
		case PropertyId::LineSpacing:
			setLineSpacing( attribute.asStyleSheetLength() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::string UICodeEditor::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Locked:
			return isLocked() ? "true" : "false";
		case PropertyId::Color:
			return getFontColor().toHexString();
		case PropertyId::TextShadowColor:
			return getFontShadowColor().toHexString();
		case PropertyId::TextShadowOffset:
			return String::fromFloat( getFontShadowOffset().x ) + " " +
				   String::fromFloat( getFontShadowOffset().y );
		case PropertyId::SelectionColor:
			return getFontSelectedColor().toHexString();
		case PropertyId::SelectionBackColor:
			return getFontSelectionBackColor().toHexString();
		case PropertyId::FontFamily:
			return NULL != getFont() ? getFont()->getName() : "";
		case PropertyId::FontSize:
			return String::format( "%.2fdp", getFontSize() );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::TextStrokeWidth:
			return String::toString( PixelDensity::dpToPx( getOutlineThickness() ) );
		case PropertyId::TextStrokeColor:
			return getOutlineColor().toHexString();
		case PropertyId::TextSelection:
			return isTextSelectionEnabled() ? "true" : "false";
		case PropertyId::LineSpacing:
			return getLineSpacing().toString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UICodeEditor::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = {
		PropertyId::Locked,			  PropertyId::Color,		   PropertyId::TextShadowColor,
		PropertyId::TextShadowOffset, PropertyId::SelectionColor,  PropertyId::SelectionBackColor,
		PropertyId::FontFamily,		  PropertyId::FontSize,		   PropertyId::FontStyle,
		PropertyId::TextStrokeWidth,  PropertyId::TextStrokeColor, PropertyId::TextSelection,
		PropertyId::LineSpacing };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

const bool& UICodeEditor::getHighlightMatchingBracket() const {
	return mHighlightMatchingBracket;
}

void UICodeEditor::setHighlightMatchingBracket( const bool& highlightMatchingBracket ) {
	if ( highlightMatchingBracket != mHighlightMatchingBracket ) {
		mHighlightMatchingBracket = highlightMatchingBracket;
		checkMatchingBrackets();
		invalidateDraw();
	}
}

const Color& UICodeEditor::getMatchingBracketColor() const {
	return mMatchingBracketColor;
}

void UICodeEditor::setMatchingBracketColor( const Color& matchingBracketColor ) {
	if ( matchingBracketColor != mMatchingBracketColor ) {
		mMatchingBracketColor = matchingBracketColor;
		invalidateDraw();
	}
}

const bool& UICodeEditor::getHighlightSelectionMatch() const {
	return mHighlightSelectionMatch;
}

void UICodeEditor::setHighlightSelectionMatch( const bool& highlightSelection ) {
	if ( highlightSelection != mHighlightSelectionMatch ) {
		mHighlightSelectionMatch = highlightSelection;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getSelectionMatchColor() const {
	return mSelectionMatchColor;
}

void UICodeEditor::setSelectionMatchColor( const Color& highlightSelectionMatchColor ) {
	if ( highlightSelectionMatchColor != mSelectionMatchColor ) {
		mSelectionMatchColor = highlightSelectionMatchColor;
		invalidateDraw();
	}
}

const bool& UICodeEditor::getEnableColorPickerOnSelection() const {
	return mEnableColorPickerOnSelection;
}

void UICodeEditor::setEnableColorPickerOnSelection( const bool& enableColorPickerOnSelection ) {
	mEnableColorPickerOnSelection = enableColorPickerOnSelection;
}

void UICodeEditor::setSyntaxDefinition( const SyntaxDefinition& definition ) {
	std::string oldLang( mDoc->getSyntaxDefinition().getLanguageName() );
	mDoc->setSyntaxDefinition( definition );
	mDoc->getHighlighter()->reset();
	invalidateDraw();
	DocSyntaxDefEvent event( this, mDoc.get(), Event::OnDocumentSyntaxDefinitionChange, oldLang,
							 mDoc->getSyntaxDefinition().getLanguageName() );
	sendEvent( &event );
}

const SyntaxDefinition& UICodeEditor::getSyntaxDefinition() const {
	return mDoc->getSyntaxDefinition();
}

void UICodeEditor::checkMatchingBrackets() {
	if ( mHighlightMatchingBracket ) {
		const std::vector<String::StringBaseType> open{ '{', '(', '[' };
		const std::vector<String::StringBaseType> close{ '}', ')', ']' };
		mMatchingBrackets = TextRange();
		TextPosition pos = mDoc->sanitizePosition( mDoc->getSelection().start() );
		TextDocumentLine& line = mDoc->line( pos.line() );
		auto isOpenIt = std::find( open.begin(), open.end(), line[pos.column()] );
		auto isCloseIt = std::find( close.begin(), close.end(), line[pos.column()] );
		if ( ( isOpenIt == open.end() && isCloseIt == close.end() ) && pos.column() > 0 ) {
			isOpenIt = std::find( open.begin(), open.end(), line[pos.column() - 1] );
			isCloseIt = std::find( close.begin(), close.end(), line[pos.column() - 1] );
			if ( isOpenIt != open.end() ) {
				pos.setColumn( pos.column() - 1 );
			} else if ( isCloseIt != close.end() ) {
				pos.setColumn( pos.column() - 1 );
			}
		}
		if ( isOpenIt != open.end() ) {
			size_t index = std::distance( open.begin(), isOpenIt );
			String::StringBaseType openBracket = open[index];
			String::StringBaseType closeBracket = close[index];
			TextPosition closePosition =
				mDoc->getMatchingBracket( pos, openBracket, closeBracket, 1 );
			mMatchingBrackets = { pos, closePosition };
		} else if ( isCloseIt != close.end() ) {
			size_t index = std::distance( close.begin(), isCloseIt );
			String::StringBaseType openBracket = open[index];
			String::StringBaseType closeBracket = close[index];
			TextPosition closePosition =
				mDoc->getMatchingBracket( pos, openBracket, closeBracket, -1 );
			mMatchingBrackets = { pos, closePosition };
		}
	}
}

Int64 UICodeEditor::getColFromXOffset( Int64 lineNumber, const Float& x ) const {
	if ( x <= 0 || !mFont || mDoc->isLoading() )
		return 0;

	TextPosition pos = mDoc->sanitizePosition( TextPosition( lineNumber, 0 ) );

	if ( mFont && !mFont->isMonospace() )
		return getLineText( pos.line() ).findCharacterFromPos( Vector2i( x, 0 ) );

	const String& line = mDoc->line( pos.line() ).getText();
	Int64 len = line.length();
	Float glyphWidth = getGlyphWidth();
	Float xOffset = 0;
	Float tabWidth = glyphWidth * mTabWidth;
	Float hTab = tabWidth * 0.5f;
	for ( int i = 0; i < len; i++ ) {
		bool isTab = ( line[i] == '\t' );
		if ( xOffset >= x ) {
			return xOffset - x > ( isTab ? hTab : glyphWidth * 0.5f ) ? eemax<Int64>( 0, i - 1 )
																	  : i;
		} else if ( isTab && ( xOffset + tabWidth > x ) ) {
			return x - xOffset > hTab ? eemin<Int64>( i + 1, line.size() - 1 ) : i;
		}
		xOffset += isTab ? tabWidth : glyphWidth;
	}
	return static_cast<Int64>( line.size() ) - 1;
}

Float UICodeEditor::getFontHeight() const {
	return mFont->getFontHeight( getCharacterSize() );
}

Float UICodeEditor::getLineHeight() const {
	return getFontHeight() + eeceil( convertLength( mLineSpacing, mSize.getWidth() ) );
}

Float UICodeEditor::getLineOffset() const {
	return eeceil( convertLength( mLineSpacing, mSize.getWidth() ) * 0.5f );
}

bool UICodeEditor::gutterSpaceExists( UICodeEditorPlugin* plugin ) const {
	for ( const auto& space : mPluginGutterSpaces ) {
		if ( space.plugin == plugin )
			return true;
	}
	return false;
}

bool UICodeEditor::topSpaceExists( UICodeEditorPlugin* plugin ) const {
	for ( const auto& space : mPluginTopSpaces ) {
		if ( space.plugin == plugin )
			return true;
	}
	return false;
}

bool UICodeEditor::registerGutterSpace( UICodeEditorPlugin* plugin, const Float& pixels,
										int order ) {
	if ( gutterSpaceExists( plugin ) )
		return false;
	mPluginGutterSpaces.push_back( { plugin, pixels, order } );
	mPluginsGutterSpace += pixels;
	std::sort( mPluginGutterSpaces.begin(), mPluginGutterSpaces.end(),
			   []( const PluginRequestedSpace& left, const PluginRequestedSpace& right ) {
				   return left.order < right.order;
			   } );
	return true;
}

bool UICodeEditor::unregisterGutterSpace( UICodeEditorPlugin* plugin ) {
	for ( size_t i = 0; i < mPluginGutterSpaces.size(); ++i ) {
		if ( mPluginGutterSpaces[i].plugin == plugin ) {
			mPluginsGutterSpace -= mPluginGutterSpaces[i].space;
			mPluginGutterSpaces.erase( mPluginGutterSpaces.begin() + i );
			return true;
		}
	}
	return false;
}

bool UICodeEditor::registerTopSpace( UICodeEditorPlugin* plugin, const Float& pixels, int order ) {
	if ( topSpaceExists( plugin ) )
		return false;
	mPluginTopSpaces.push_back( { plugin, pixels, order } );
	mPluginsTopSpace += pixels;
	std::sort( mPluginTopSpaces.begin(), mPluginTopSpaces.end(),
			   []( const PluginRequestedSpace& left, const PluginRequestedSpace& right ) {
				   return left.order < right.order;
			   } );
	return true;
}

bool UICodeEditor::unregisterTopSpace( UICodeEditorPlugin* plugin ) {
	for ( size_t i = 0; i < mPluginTopSpaces.size(); ++i ) {
		if ( mPluginTopSpaces[i].plugin == plugin ) {
			mPluginsTopSpace -= mPluginTopSpaces[i].space;
			mPluginTopSpaces.erase( mPluginTopSpaces.begin() + i );
			return true;
		}
	}
	return false;
}

Float UICodeEditor::getCharacterSize() const {
	return PixelDensity::dpToPx( mFontStyleConfig.getFontCharacterSize() );
}

Float UICodeEditor::getGlyphWidth() const {
	return mGlyphWidth;
}

void UICodeEditor::udpateGlyphWidth() {
	mGlyphWidth = mFont->getGlyph( '@', getCharacterSize(), false ).advance;
	invalidateLongestLineWidth();
}

Drawable* UICodeEditor::findIcon( const std::string& name ) {
	UIIcon* icon = getUISceneNode()->findIcon( name );
	if ( icon )
		return icon->getSize( mMenuIconSize );
	return nullptr;
}

const bool& UICodeEditor::getColorPreview() const {
	return mColorPreview;
}

void UICodeEditor::setColorPreview( bool colorPreview ) {
	mColorPreview = colorPreview;
}

void UICodeEditor::resetCursor() {
	mCursorVisible = true;
	mBlinkTimer.restart();
}

TextPosition UICodeEditor::moveToLineOffset( const TextPosition& position, int offset,
											 const size_t& cursorIdx ) {
	auto& xo = mLastXOffset[cursorIdx];
	if ( xo.position != position )
		xo.offset = getXOffsetColSanitized( position );
	xo.position.setLine( position.line() + offset );
	xo.position.setColumn( getColFromXOffset( position.line() + offset, xo.offset ) );
	return xo.position;
}

void UICodeEditor::moveToPreviousLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition position = mDoc->getSelections()[i].start();
		if ( position.line() == 0 ) {
			mDoc->setSelection( i, mDoc->startOfDoc(), mDoc->startOfDoc() );
		} else {
			mDoc->moveTo( i, moveToLineOffset( position, -1, i ) );
		}
	}
	mDoc->mergeSelection();
}

void UICodeEditor::moveToNextLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition position = mDoc->getSelections()[i].start();
		if ( position.line() == (Int64)mDoc->linesCount() - 1 ) {
			mDoc->setSelection( i, mDoc->endOfDoc(), mDoc->endOfDoc() );
		} else {
			mDoc->moveTo( i, moveToLineOffset( position, 1, i ) );
		}
	}
	mDoc->mergeSelection();
}

void UICodeEditor::selectToPreviousLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition position = mDoc->getSelectionIndex( i ).start();
		if ( position.line() == 0 ) {
			mDoc->selectTo( i, mDoc->startOfDoc() );
		} else {
			mDoc->selectTo( i, moveToLineOffset( position, -1 ) );
		}
	}
}

void UICodeEditor::selectToNextLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition position = mDoc->getSelectionIndex( i ).start();
		if ( position.line() == (Int64)mDoc->linesCount() - 1 ) {
			mDoc->selectTo( i, mDoc->endOfDoc() );
		} else {
			mDoc->selectTo( i, moveToLineOffset( position, 1 ) );
		}
	}
}

void UICodeEditor::moveScrollUp() {
	setScrollY( mScroll.y - getLineHeight() );
}

void UICodeEditor::moveScrollDown() {
	setScrollY( mScroll.y + getLineHeight() );
}

void UICodeEditor::indent() {
	UIEventDispatcher* eventDispatcher =
		static_cast<UIEventDispatcher*>( getUISceneNode()->getEventDispatcher() );
	if ( !eventDispatcher->justGainedFocus() ) {
		mDoc->indent();
	}
}

void UICodeEditor::unindent() {
	UIEventDispatcher* eventDispatcher =
		static_cast<UIEventDispatcher*>( getUISceneNode()->getEventDispatcher() );
	if ( !eventDispatcher->justGainedFocus() ) {
		mDoc->unindent();
	}
}

void UICodeEditor::copy() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc->getAllSelectedText().toUtf8() );
}

void UICodeEditor::cut() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc->getAllSelectedText().toUtf8() );
	mDoc->deleteSelection();
}

void UICodeEditor::paste() {
	mDoc->textInput( getUISceneNode()->getWindow()->getClipboard()->getText() );
	sendCommonEvent( Event::OnTextPasted );
}

void UICodeEditor::fontSizeGrow() {
	Float line = mScroll.y / getLineHeight();
	Float col = mScroll.x / getGlyphWidth();
	mFontStyleConfig.CharacterSize = eemin<Float>( 96, mFontStyleConfig.CharacterSize + 1 );
	onFontChanged();
	updateLongestLineWidth();
	setScrollY( getLineHeight() * line );
	setScrollX( getGlyphWidth() * col );
	invalidateDraw();
}

void UICodeEditor::fontSizeShrink() {
	Float line = mScroll.y / getLineHeight();
	Float col = mScroll.x / getGlyphWidth();
	mFontStyleConfig.CharacterSize = eemax<Float>( 4, mFontStyleConfig.CharacterSize - 1 );
	onFontChanged();
	updateLongestLineWidth();
	setScrollY( getLineHeight() * line );
	setScrollX( getGlyphWidth() * col );
	invalidateDraw();
}

void UICodeEditor::fontSizeReset() {
	Float line = mScroll.y / getLineHeight();
	Float col = mScroll.x / getGlyphWidth();
	setFontSize( mFontSize );
	updateLongestLineWidth();
	setScrollY( getLineHeight() * line );
	setScrollX( getGlyphWidth() * col );
}

const bool& UICodeEditor::getShowWhitespaces() const {
	return mShowWhitespaces;
}

void UICodeEditor::setShowWhitespaces( const bool& showWhitespaces ) {
	if ( mShowWhitespaces != showWhitespaces ) {
		mShowWhitespaces = showWhitespaces;
		invalidateDraw();
	}
}

const TextSearchParams& UICodeEditor::getHighlightWord() const {
	return mHighlightWord;
}

void UICodeEditor::updateHighlightWordCache() {
	if ( getUISceneNode()->hasThreadPool() ) {
		Uint64 tag = reinterpret_cast<Uint64>( this );
		getUISceneNode()->getThreadPool()->removeWithTag( tag );
		getUISceneNode()->getThreadPool()->run(
			[this]() {
				mHighlightWordProcessing = true;
				mHighlightWordCache = mDoc->findAll(
					mHighlightWord.text, mHighlightWord.caseSensitive, mHighlightWord.wholeWord,
					mHighlightWord.type, mHighlightWord.range );
			},
			[this]( const auto& ) { mHighlightWordProcessing = false; }, tag );
	} else {
		mHighlightWordCache =
			mDoc->findAll( mHighlightWord.text, mHighlightWord.caseSensitive,
						   mHighlightWord.wholeWord, mHighlightWord.type, mHighlightWord.range );
	}
}

void UICodeEditor::setHighlightWord( const TextSearchParams& highlightWord ) {
	if ( mHighlightWord != highlightWord ) {
		mHighlightWord = highlightWord;
		updateHighlightWordCache();
		invalidateDraw();
	}
}

const TextRange& UICodeEditor::getHighlightTextRange() const {
	return mHighlightTextRange;
}

void UICodeEditor::setHighlightTextRange( const TextRange& highlightSelection ) {
	if ( highlightSelection != mHighlightTextRange ) {
		mHighlightTextRange = highlightSelection;
		invalidateDraw();
	}
}

void UICodeEditor::registerPlugin( UICodeEditorPlugin* plugin ) {
	auto it = std::find( mPlugins.begin(), mPlugins.end(), plugin );
	if ( it == mPlugins.end() ) {
		mPlugins.push_back( plugin );
		plugin->onRegister( this );
	}
}

void UICodeEditor::unregisterPlugin( UICodeEditorPlugin* plugin ) {
	auto it = std::find( mPlugins.begin(), mPlugins.end(), plugin );
	if ( it != mPlugins.end() ) {
		mPlugins.erase( it );
		plugin->onUnregister( this );
	}
}

const Time& UICodeEditor::getFindLongestLineWidthUpdateFrequency() const {
	return mFindLongestLineWidthUpdateFrequency;
}

void UICodeEditor::setFindLongestLineWidthUpdateFrequency(
	const Time& findLongestLineWidthUpdateFrequency ) {
	mFindLongestLineWidthUpdateFrequency = findLongestLineWidthUpdateFrequency;
}

const bool& UICodeEditor::getHorizontalScrollBarEnabled() const {
	return mHorizontalScrollBarEnabled;
}

void UICodeEditor::setHorizontalScrollBarEnabled( const bool& horizontalScrollBarEnabled ) {
	if ( horizontalScrollBarEnabled != mHorizontalScrollBarEnabled ) {
		mHorizontalScrollBarEnabled = horizontalScrollBarEnabled;
		invalidateLongestLineWidth();
		updateScrollBar();
	}
}

bool UICodeEditor::getVerticalScrollBarEnabled() const {
	return mVerticalScrollBarEnabled;
}

void UICodeEditor::setVerticalScrollBarEnabled( const bool& verticalScrollBarEnabled ) {
	if ( verticalScrollBarEnabled != mVerticalScrollBarEnabled ) {
		mVerticalScrollBarEnabled = verticalScrollBarEnabled;
		updateScrollBar();
	}
}

void UICodeEditor::drawMatchingBrackets( const Vector2f& startScroll, const Float& lineHeight ) {
	if ( mMatchingBrackets.isValid() ) {
		Primitives primitive;
		primitive.setForceDraw( false );
		primitive.setColor( Color( mMatchingBracketColor ).blendAlpha( mAlpha ) );
		auto drawBracket = [&]( const TextPosition& pos ) {
			primitive.drawRectangle( Rectf( Vector2f( startScroll.x + getXOffsetCol( pos ),
													  startScroll.y + pos.line() * lineHeight ),
											Sizef( getGlyphWidth(), lineHeight ) ) );
		};
		drawBracket( mMatchingBrackets.start() );
		drawBracket( mMatchingBrackets.end() );
		primitive.setForceDraw( true );
	}
}

void UICodeEditor::drawSelectionMatch( const std::pair<int, int>& lineRange,
									   const Vector2f& startScroll, const Float& lineHeight ) {
	if ( !mDoc->hasSelection() )
		return;
	TextRange selection = mDoc->getSelection( true );
	const String& selectionLine = mDoc->line( selection.start().line() ).getText();
	if ( selection.start().column() >= 0 &&
		 selection.start().column() < (Int64)selectionLine.size() &&
		 selection.end().column() >= 0 && selection.end().column() < (Int64)selectionLine.size() ) {
		String text( selectionLine.substr(
			selection.start().column(), selection.end().column() - selection.start().column() ) );
		if ( !text.empty() )
			drawWordMatch( text, lineRange, startScroll, lineHeight, true );
	}
}

void UICodeEditor::drawWordRanges( const TextRanges& ranges, const std::pair<int, int>& lineRange,
								   const Vector2f& startScroll, const Float& lineHeight,
								   bool ignoreSelectionMatch ) {
	if ( ranges.empty() )
		return;
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( mSelectionMatchColor ).blendAlpha( mAlpha ) );
	TextRange selection = mDoc->getSelection( true );

	for ( const auto& range : ranges ) {
		if ( !( range.start().line() >= lineRange.first &&
				range.end().line() <= lineRange.second ) )
			continue;

		if ( ignoreSelectionMatch && selection.inSameLine() &&
			 selection.start().line() == range.start().line() &&
			 selection.start().column() == range.start().column() ) {
			continue;
		}

		if ( !range.inSameLine() )
			continue;

		Rectf selRect;
		Int64 startCol = range.start().column();
		Int64 endCol = range.end().column();
		selRect.Top = startScroll.y + range.start().line() * lineHeight;
		selRect.Bottom = selRect.Top + lineHeight;
		selRect.Left = startScroll.x + getXOffsetCol( { range.start().line(), startCol } );
		selRect.Right = startScroll.x + getXOffsetCol( { range.start().line(), endCol } );
		primitives.drawRectangle( selRect );
	}

	primitives.setForceDraw( true );
}

void UICodeEditor::drawWordMatch( const String& text, const std::pair<int, int>& lineRange,
								  const Vector2f& startScroll, const Float& lineHeight,
								  bool ignoreSelectionMatch ) {
	if ( text.empty() )
		return;
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( mSelectionMatchColor ).blendAlpha( mAlpha ) );
	for ( auto ln = lineRange.first; ln <= lineRange.second; ln++ ) {
		const String& line = mDoc->line( ln ).getText();
		size_t pos = 0;
		// Skip ridiculously long lines.
		if ( line.size() > 300 )
			continue;

		do {
			pos = line.find( text, pos );
			if ( pos != String::InvalidPos ) {
				if ( ignoreSelectionMatch ) {
					TextRange selection = mDoc->getSelection( true );
					if ( selection.inSameLine() && selection.start().line() == ln &&
						 selection.start().column() == (Int64)pos ) {
						pos = selection.end().column();
						continue;
					}
				}

				Rectf selRect;
				Int64 startCol = pos;
				Int64 endCol = pos + text.size();
				selRect.Top = startScroll.y + ln * lineHeight;
				selRect.Bottom = selRect.Top + lineHeight;
				selRect.Left = startScroll.x + getXOffsetCol( { ln, startCol } );
				selRect.Right = startScroll.x + getXOffsetCol( { ln, endCol } );
				primitives.drawRectangle( selRect );
				pos = endCol;
			} else {
				break;
			}
		} while ( true );
	}
	primitives.setForceDraw( true );
}

void UICodeEditor::drawLineText( const Int64& line, Vector2f position, const Float& fontSize,
								 const Float& lineHeight ) {
	Vector2f originalPosition( position );
	auto& tokens = mDoc->getHighlighter()->getLine( line );
	const String& strLine = mDoc->line( line ).getText();
	Primitives primitives;
	Int64 curChar = 0;
	Int64 maxWidth = eeceil( mSize.getWidth() / getGlyphWidth() + 1 );
	bool isMonospace = mFont->isMonospace();
	bool isFallbackFont = false;
	bool isEmojiFallbackFont = false;
	Float lineOffset = getLineOffset();
	size_t pos = 0;
	if ( mDoc->mightBeBinary() && mFont->getType() == FontType::TTF ) {
		FontTrueType* ttf = static_cast<FontTrueType*>( mFont );
		isFallbackFont = ttf->isFallbackFontEnabled();
		isEmojiFallbackFont = ttf->isEmojiFallbackEnabled();
		ttf->setEnableFallbackFont( false );
		ttf->setEnableEmojiFallback( false );
	}
	for ( const auto& token : tokens ) {
		String text( pos < strLine.size() ? strLine.substr( pos, token.len ) : String() );
		pos += token.len;

		Float textWidth = isMonospace ? getTextWidth( text ) : 0;
		if ( !isMonospace || ( position.x + textWidth >= mScreenPos.x &&
							   position.x <= mScreenPos.x + mSize.getWidth() ) ) {
			Int64 curCharsWidth = text.size();
			Int64 curPositionChar = eefloor( mScroll.x / getGlyphWidth() );
			Float curMaxPositionChar = curPositionChar + maxWidth;
			Text txt( "", mFont, fontSize );
			txt.setTabWidth( mTabWidth );
			const SyntaxColorScheme::Style& style = mColorScheme.getSyntaxStyle( token.type );
			txt.setStyleConfig( mFontStyleConfig );
			if ( style.style )
				txt.setStyle( style.style );
			txt.setColor( Color( style.color ).blendAlpha( mAlpha ) );

			if ( isMonospace )
				txt.setDisableCacheWidth( true );

			if ( mHandShown && mLinkPosition.isValid() && mLinkPosition.inSameLine() &&
				 mLinkPosition.start().line() == line ) {
				if ( mLinkPosition.start().column() >= curChar &&
					 mLinkPosition.end().column() <= curChar + curCharsWidth ) {
					size_t linkPos = text.find( mLink );
					if ( linkPos != String::InvalidPos ) {
						String beforeString( text.substr( 0, linkPos ) );
						String afterString( text.substr( linkPos + mLink.size() ) );

						Float offset = 0.f;
						Uint32 lineStyle = txt.getStyle();

						if ( !beforeString.empty() ) {
							Float beforeWidth = getTextWidth( beforeString );
							if ( style.background != Color::Transparent ) {
								primitives.setColor(
									Color( style.background ).blendAlpha( mAlpha ) );
								primitives.drawRectangle(
									Rectf( position, Sizef( beforeWidth, lineHeight ) ) );
							}
							txt.setString( beforeString );
							txt.draw( position.x, position.y + lineOffset );
							offset += beforeWidth;
						}

						SyntaxColorScheme::Style linkStyle = style;

						if ( mColorScheme.hasSyntaxStyle( "link_hover" ) ) {
							linkStyle = mColorScheme.getSyntaxStyle( "link_hover" );
							if ( linkStyle.color != Color::Transparent )
								txt.setColor( Color( linkStyle.color ).blendAlpha( mAlpha ) );
							txt.setStyle( linkStyle.style );
						} else {
							txt.setStyle( ( lineStyle & Text::Underlined )
											  ? ( lineStyle | Text::Bold )
											  : ( lineStyle | Text::Underlined ) );
						}

						Float linkWidth = getTextWidth( mLink );
						if ( linkStyle.background != Color::Transparent ) {
							primitives.setColor(
								Color( linkStyle.background ).blendAlpha( mAlpha ) );
							primitives.drawRectangle(
								Rectf( Vector2f( position.x + offset, position.y ),
									   Sizef( linkWidth, lineHeight ) ) );
						}
						txt.setString( mLink );
						txt.draw( position.x + offset, position.y + lineOffset );
						offset += linkWidth;

						if ( !afterString.empty() ) {
							Float afterWidth = getTextWidth( afterString );
							if ( style.background != Color::Transparent ) {
								primitives.setColor(
									Color( style.background ).blendAlpha( mAlpha ) );
								primitives.drawRectangle(
									Rectf( Vector2f( position.x + offset, position.y ),
										   Sizef( afterWidth, lineHeight ) ) );
							}
							txt.setColor( Color( style.color ).blendAlpha( mAlpha ) );
							txt.setStyle( lineStyle );
							txt.setString( afterString );
							txt.draw( position.x + offset, position.y + lineOffset );
							offset += afterWidth;
						}

						if ( !isMonospace )
							textWidth = offset;

						position.x += textWidth;
						curChar += characterWidth( text );
						continue;
					}
				}
			}

			if ( style.background != Color::Transparent ) {
				primitives.setColor( Color( style.background ).blendAlpha( mAlpha ) );
				primitives.drawRectangle( Rectf( position, Sizef( textWidth, lineHeight ) ) );
			}

			if ( isMonospace && curPositionChar + curChar + curCharsWidth > curMaxPositionChar ) {
				if ( curChar < curPositionChar ) {
					Int64 charsToVisible = curPositionChar - curChar;
					Int64 start = eemax( (Int64)0, curPositionChar - curChar );
					Int64 minimumCharsToCoverScreen = maxWidth + charsToVisible - start;
					Int64 totalChars = curCharsWidth - start;
					Int64 end = eemin( totalChars, minimumCharsToCoverScreen );
					if ( curCharsWidth >= charsToVisible ) {
						txt.setString( text.substr( start, end ) );
						txt.draw( position.x + start * getGlyphWidth(), position.y + lineOffset );
						if ( minimumCharsToCoverScreen == end )
							break;
					}
				} else {
					txt.setString( text.substr( 0, eemin( curCharsWidth, maxWidth ) ) );
					txt.draw( position.x, position.y + lineOffset );
				}
			} else {
				txt.setString( text );
				txt.draw( position.x, position.y + lineOffset );
			}

			if ( !isMonospace )
				textWidth = txt.getTextWidth();
		} else if ( position.x > mScreenPos.x + mSize.getWidth() ) {
			break;
		}

		position.x += textWidth;
		curChar += characterWidth( text );
	}

	if ( mDoc->mightBeBinary() && mFont->getType() == FontType::TTF ) {
		FontTrueType* ttf = static_cast<FontTrueType*>( mFont );
		ttf->setEnableFallbackFont( isFallbackFont );
		ttf->setEnableEmojiFallback( isEmojiFallbackFont );
	}
}

void UICodeEditor::drawTextRange( const TextRange& range, const std::pair<int, int>& lineRange,
								  const Vector2f& startScroll, const Float& lineHeight,
								  const Color& backgroundColor ) {
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( backgroundColor ).blendAlpha( mAlpha ) );

	int startLine = eemax<int>( lineRange.first, range.start().line() );
	int endLine = eemin<int>( lineRange.second, range.end().line() );

	for ( auto ln = startLine; ln <= endLine; ln++ ) {
		const String& line = mDoc->line( ln ).getText();
		Rectf selRect;
		selRect.Top = startScroll.y + ln * lineHeight;
		selRect.Bottom = selRect.Top + lineHeight;
		if ( range.start().line() == ln ) {
			selRect.Left = startScroll.x + getXOffsetCol( { ln, range.start().column() } );
			if ( range.end().line() == ln ) {
				selRect.Right = startScroll.x + getXOffsetCol( { ln, range.end().column() } );
			} else {
				selRect.Right =
					startScroll.x + getXOffsetCol( { ln, static_cast<Int64>( line.length() ) } );
			}
		} else if ( range.end().line() == ln ) {
			selRect.Left = startScroll.x + getXOffsetCol( { ln, 0 } );
			selRect.Right = startScroll.x + getXOffsetCol( { ln, range.end().column() } );
		} else {
			selRect.Left = startScroll.x + getXOffsetCol( { ln, 0 } );
			selRect.Right =
				startScroll.x + getXOffsetCol( { ln, static_cast<Int64>( line.length() ) } );
		}

		primitives.drawRectangle( selRect );
	}
	primitives.setForceDraw( true );
}

void UICodeEditor::drawLineNumbers( const std::pair<int, int>& lineRange,
									const Vector2f& startScroll, const Vector2f& screenStart,
									const Float& lineHeight, const Float& lineNumberWidth,
									const int& lineNumberDigits, const Float& fontSize ) {
	Primitives primitives;
	primitives.setColor( Color( mLineNumberBackgroundColor ).blendAlpha( mAlpha ) );
	primitives.drawRectangle( Rectf( screenStart, Sizef( lineNumberWidth, mSize.getHeight() ) ) );
	TextRange selection = mDoc->getSelection( true );
	Float lineOffset = getLineOffset();

	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		String pos;
		if ( mShowLinesRelativePosition && selection.start().line() != i ) {
			pos = String( String::toString( eeabs( i - selection.start().line() ) ) )
					  .padLeft( lineNumberDigits, ' ' );
		} else {
			pos = String( String::toString( i + 1 ) ).padLeft( lineNumberDigits, ' ' );
		}
		Text line( std::move( pos ), mFont, fontSize );
		line.setStyleConfig( mFontStyleConfig );
		line.setColor( ( i >= selection.start().line() && i <= selection.end().line() )
						   ? mLineNumberActiveFontColor
						   : mLineNumberFontColor );
		line.draw( screenStart.x + mLineNumberPaddingLeft,
				   startScroll.y + lineHeight * (double)i + lineOffset );
	}
}

void UICodeEditor::drawColorPreview( const Vector2f& startScroll, const Float& lineHeight ) {
	Primitives primitives;
	primitives.setColor( mPreviewColor );
	Float startX = getXOffsetCol( mPreviewColorRange.start() );
	Float endX = getXOffsetCol( mPreviewColorRange.end() );
	primitives.drawRectangle( Rectf(
		Vector2f( startScroll.x + mScroll.x + startX,
				  startScroll.y + mPreviewColorRange.start().line() * lineHeight + lineHeight ),
		Sizef( endX - startX, lineHeight * 2 ) ) );
}

void UICodeEditor::drawWhitespaces( const std::pair<int, int>& lineRange,
									const Vector2f& startScroll, const Float& lineHeight ) {
	Float tabWidth = getTextWidth( "\t" );
	Float glyphW = getGlyphWidth();
	Color color( Color( mWhitespaceColor ).blendAlpha( mAlpha ) );
	unsigned int fontSize = getCharacterSize();
	// We use the GlyphDrawable since can batch the draw calls instead of Text.
	GlyphDrawable* adv = mFont->getGlyphDrawable( 187 /*'»'*/, fontSize );
	GlyphDrawable* cpoint = mFont->getGlyphDrawable( 183 /*'·'*/, fontSize );
	Float tabCenter = ( tabWidth - adv->getPixelsSize().getWidth() ) * 0.5f;
	adv->setDrawMode( GlyphDrawable::DrawMode::Text );
	cpoint->setDrawMode( GlyphDrawable::DrawMode::Text );
	adv->setColor( color );
	cpoint->setColor( color );
	for ( int index = lineRange.first; index <= lineRange.second; index++ ) {
		Vector2f position( { startScroll.x, startScroll.y + lineHeight * index } );
		const auto& text = mDoc->line( index ).getText();
		for ( size_t i = 0; i < text.size(); i++ ) {
			if ( position.x + mScroll.x + ( text[i] == '\t' ? tabWidth : glyphW ) >= mScreenPos.x &&
				 position.x <= mScreenPos.x + mScroll.x + mSize.getWidth() ) {
				if ( ' ' == text[i] ) {
					cpoint->draw( Vector2f( position.x, position.y ) );
					position.x += glyphW;
				} else if ( '\t' == text[i] ) {
					adv->draw( Vector2f( position.x + tabCenter, position.y ) );
					position.x += tabWidth;
				} else {
					position.x += glyphW;
				}
			} else if ( position.x > mScreenPos.x + mSize.getWidth() ) {
				break;
			} else {
				position.x += glyphW;
			}
		}
	}
}

static Int64 getLineSpaces( TextDocument& doc, int line, int dir, int indentSize ) {
	if ( line < 0 || line >= (int)doc.linesCount() )
		return -1;
	const auto& text = doc.line( line ).getText();
	if ( text.size() <= 1 )
		return -1;
	auto s = text.find_first_not_of( " \t\n" );
	if ( s == std::string::npos )
		return -getLineSpaces( doc, line + dir, dir, indentSize );
	int n = 0;
	for ( size_t i = 0; i < s; ++i )
		n += text[i] == ' ' ? 1 : indentSize;
	return n;
}

static Int64 getLineIndentGuideSpaces( TextDocument& doc, int line, int indentSize ) {
	if ( doc.line( line ).getText().find_first_not_of( " \t\n" ) == std::string::npos )
		return eemax( getLineSpaces( doc, line - 1, -1, indentSize ),
					  getLineSpaces( doc, line + 1, 1, indentSize ) );
	return getLineSpaces( doc, line, 0, indentSize );
}

void UICodeEditor::drawIndentationGuides( const std::pair<int, int>& lineRange,
										  const Vector2f& startScroll, const Float& lineHeight ) {
	Primitives p;
	p.setForceDraw( false );
	Float w = eefloor( PixelDensity::dpToPx( 1 ) );
	String idt( mDoc->getIndentString() );
	int spaceW = getTextWidth( " " );
	p.setColor( Color( mWhitespaceColor ).blendAlpha( mAlpha ) );
	int indentSize = mDoc->getIndentType() == TextDocument::IndentType::IndentTabs
						 ? getTabWidth()
						 : mDoc->getIndentWidth();
	for ( int index = lineRange.first; index <= lineRange.second; index++ ) {
		Vector2f position( { startScroll.x, startScroll.y + lineHeight * index } );
		int spaces = getLineIndentGuideSpaces( *mDoc.get(), index, indentSize );
		for ( int i = 0; i < spaces; i += indentSize )
			p.drawRectangle( Rectf( { position.x + spaceW * i, position.y }, { w, lineHeight } ) );
	}
}

void UICodeEditor::drawLineEndings( const std::pair<int, int>& lineRange,
									const Vector2f& startScroll, const Float& lineHeight ) {

	Color color( Color( mWhitespaceColor ).blendAlpha( mAlpha ) );
	unsigned int fontSize = getCharacterSize();
	GlyphDrawable* nl = mFont->getGlyphDrawable( 8628 /*'↴'*/, fontSize );
	if ( nl->getPixelsSize() == Sizef::Zero )
		nl = mFont->getGlyphDrawable( 172 /* '¬'*/, fontSize );
	nl->setDrawMode( GlyphDrawable::DrawMode::Text );
	nl->setColor( color );
	for ( int index = lineRange.first; index <= lineRange.second; index++ ) {
		Vector2f position( { startScroll.x + getLineWidth( index ) - getGlyphWidth(),
							 startScroll.y + lineHeight * index } );
		nl->draw( Vector2f( position.x, position.y ) );
	}
}

void UICodeEditor::registerCommands() {
	mDoc->setCommand( "move-to-previous-line", [&] { moveToPreviousLine(); } );
	mDoc->setCommand( "move-to-next-line", [&] { moveToNextLine(); } );
	mDoc->setCommand( "select-to-previous-line", [&] { selectToPreviousLine(); } );
	mDoc->setCommand( "select-to-next-line", [&] { selectToNextLine(); } );
	mDoc->setCommand( "move-scroll-up", [&] { moveScrollUp(); } );
	mDoc->setCommand( "move-scroll-down", [&] { moveScrollDown(); } );
	mDoc->setCommand( "indent", [&] { indent(); } );
	mDoc->setCommand( "unindent", [&] { unindent(); } );
	mDoc->setCommand( "copy", [&] { copy(); } );
	mDoc->setCommand( "cut", [&] { cut(); } );
	mDoc->setCommand( "paste", [&] { paste(); } );
	mDoc->setCommand( "font-size-grow", [&] { fontSizeGrow(); } );
	mDoc->setCommand( "font-size-shrink", [&] { fontSizeShrink(); } );
	mDoc->setCommand( "font-size-reset", [&] { fontSizeReset(); } );
	mDoc->setCommand( "lock", [&] { setLocked( true ); } );
	mDoc->setCommand( "unlock", [&] { setLocked( false ); } );
	mDoc->setCommand( "lock-toggle", [&] { setLocked( !isLocked() ); } );
	mDoc->setCommand( "open-containing-folder", [&] { openContainingFolder(); } );
	mDoc->setCommand( "copy-containing-folder-path", [&] { copyContainingFolderPath(); } );
	mDoc->setCommand( "copy-file-path", [&] { copyFilePath(); } );
	mDoc->setCommand( "find-replace", [&] { showFindReplace(); } );
	mDoc->setCommand( "open-context-menu", [&] { createContextMenu(); } );
	mUnlockedCmd.insert( { "copy", "select-all" } );
}

void UICodeEditor::showFindReplace() {
	UISceneNode* curSceneNode = SceneManager::instance()->getUISceneNode();
	if ( mUISceneNode != curSceneNode )
		SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );

	if ( !mFindReplaceEnabled )
		return;
	if ( nullptr == mFindReplace )
		mFindReplace = UIDocFindReplace::New( this, mDoc );
	mFindReplace->show();

	if ( mUISceneNode != curSceneNode )
		SceneManager::instance()->setCurrentUISceneNode( curSceneNode );
}

void UICodeEditor::registerKeybindings() {
	mKeyBindings.addKeybinds( getDefaultKeybindings() );
}

void UICodeEditor::onCursorPosChange() {
	sendCommonEvent( Event::OnCursorPosChange );
	invalidateDraw();
}

static bool checkHexa( const std::string& hexStr ) {
	for ( size_t i = 1; i < hexStr.size(); ++i )
		if ( !isxdigit( hexStr[i] ) )
			return false;
	return true;
}

void UICodeEditor::checkMouseOverColor( const Vector2i& position ) {
	if ( !mColorPreview || mDoc->isLoading() )
		return;
	TextPosition pos( resolveScreenPosition( position.asFloat() ) );
	const String& line = mDoc->line( pos.line() ).getText();
	if ( pos.column() >= (Int64)line.size() - 1 ) {
		resetPreviewColor();
		return;
	}
	TextPosition start( mDoc->previousWordBoundary( pos ) );
	if ( start.column() > 0 && start.column() < (Int64)line.size() ) {
		TextPosition end( mDoc->nextWordBoundary( pos ) );
		TextRange wordPos = { { start.line(), start.column() - 1 }, end };
		String word = mDoc->getText( wordPos );
		bool found = false;
		if ( word[0] == '#' && ( word.size() == 7 || word.size() == 9 ) ) {
			if ( checkHexa( word ) )
				found = true;
		} else {
			wordPos = { start, end };
			word = mDoc->getText( wordPos );
			if ( end.column() < (Int64)line.size() && line[end.column()] == '(' &&
				 ( "rgb" == word || "rgba" == word || "hsl" == word || "hsv" == word ||
				   "hsla" == word || "hsva" == word ) ) {
				const String& text = mDoc->line( start.line() ).getText();
				size_t endFun = String::findCloseBracket( text, end.column(), '(', ')' );
				if ( endFun != std::string::npos ) {
					word = word + text.substr( end.column(), endFun - end.column() + 1 );
					if ( word.find( "--" ) == String::InvalidPos ) {
						found = true;
						wordPos = { wordPos.start(), { wordPos.end().line(), (Int64)endFun + 1 } };
					}
				}
			}
		}
		if ( found ) {
			mPreviewColor = Color::fromString( word );
			mPreviewColorRange = wordPos;
			invalidateDraw();
		} else {
			resetPreviewColor();
		}
	} else if ( mPreviewColorRange.isValid() ) {
		resetPreviewColor();
	}
}

String UICodeEditor::checkMouseOverLink( const Vector2i& position ) {
	if ( !mInteractiveLinks || !getUISceneNode()->getWindow()->getInput()->isControlPressed() )
		return resetLinkOver();

	TextPosition pos( resolveScreenPosition( position.asFloat(), false ) );
	if ( mDoc->getChar( pos ) == '\n' )
		return resetLinkOver();

	if ( pos.line() > (Int64)mDoc->linesCount() )
		return resetLinkOver();

	const String& line = mDoc->line( pos.line() ).getText();
	if ( pos.column() >= (Int64)line.size() - 1 )
		return resetLinkOver();

	TextPosition startB( mDoc->previousSpaceBoundaryInLine( pos ) );
	TextPosition endB( mDoc->nextSpaceBoundaryInLine( pos ) );

	if ( startB.column() >= (Int64)line.size() || endB.column() >= (Int64)line.size() )
		return resetLinkOver();

	if ( pos.column() <= startB.column() || pos.column() >= endB.column() )
		return resetLinkOver();

	String partialLine( line.substr( startB.column(), endB.column() ) );

	LuaPattern words( LuaPattern::getURIPattern() );
	int start, end = 0;
	std::string linkStr( partialLine.toUtf8() );

	int offset = 0;
	std::vector<std::pair<int, int>> links;
	do {
		if ( words.find( linkStr, start, end, offset ) ) {
			links.emplace_back( std::make_pair( start, end ) );
			offset = end;
		}
	} while ( start != end );

	if ( !links.empty() ) {
		for ( const auto& link : links ) {
			if ( pos.column() >= startB.column() + link.first &&
				 pos.column() <= startB.column() + link.second ) {
				getUISceneNode()->setCursor( Cursor::Hand );
				mHandShown = true;
				mLinkPosition = {
					{ startB.line(), static_cast<Int64>( characterWidth(
										 mDoc->line( startB.line() )
											 .getText()
											 .substr( 0, startB.column() + link.first ) ) ) },
					{ startB.line(), startB.column() + link.second } };
				mLink = String( linkStr.substr( link.first, link.second - link.first ) );
				invalidateDraw();
				return mLink;
			}
		}
	}

	return resetLinkOver();
}

String UICodeEditor::resetLinkOver() {
	if ( mHandShown )
		invalidateDraw();
	mHandShown = false;
	getUISceneNode()->setCursor( !mLocked ? Cursor::IBeam : Cursor::Arrow );
	mLinkPosition = TextRange();
	mLink.clear();
	return "";
}

void UICodeEditor::resetPreviewColor() {
	if ( mPreviewColorRange != TextRange() ) {
		mPreviewColorRange = TextRange();
		mPreviewColor = Color::Transparent;
		invalidateDraw();
	}
}

Float UICodeEditor::getMinimapWidth() const {
	Float w = PixelDensity::dpToPx( mMinimapConfig.width );
	// Max [mMinimapConfig.maxPercentWidth]% of the editor view width
	if ( w / getPixelsSize().getWidth() > mMinimapConfig.maxPercentWidth )
		w = getPixelsSize().getWidth() * mMinimapConfig.maxPercentWidth;
	return w;
}

Rectf UICodeEditor::getMinimapRect( const Vector2f& start ) const {
	Float w = getMinimapWidth();
	Float h = getPixelsSize().getHeight() -
			  ( mHScrollBar->isVisible() ? mHScrollBar->getPixelsSize().getHeight() : 0.f );
	return Rectf(
		{ start.x + getPixelsSize().getWidth() - w -
			  ( mVScrollBar->isVisible() ? mVScrollBar->getPixelsSize().getWidth() : 0.f ),
		  start.y },
		Sizef( w, h ) );
}

void UICodeEditor::drawMinimap( const Vector2f& start,
								const std::pair<Uint64, Uint64>& lineRange ) {
	Float charHeight = PixelDensity::getPixelDensity() * mMinimapConfig.scale;
	Float charSpacing =
		eemax( 1.f, eefloor( 0.8 * PixelDensity::getPixelDensity() * mMinimapConfig.scale ) );
	Float lineSpacing = getMinimapLineSpacing();
	Rectf rect( getMinimapRect( start ) );
	int visibleLinesCount = ( lineRange.second - lineRange.first );
	int visibleLinesStart = lineRange.first;
	Float scrollerHeight = visibleLinesCount * lineSpacing;
	int lineCount = mDoc->linesCount();
	Float visibleY = rect.Top + visibleLinesStart * lineSpacing;
	int maxMinmapLines = eefloor( rect.getHeight() / lineSpacing );
	int minimapStartLine = 0;

	if ( isMinimapFileTooLarge() ) {
		Float scrollPos = ( visibleLinesStart - 1 ) / (Float)( lineCount - visibleLinesCount - 1 );
		scrollPos = eeclamp( scrollPos, 0.f, 1.f );
		Float scrollPosPixels = scrollPos * ( rect.getHeight() - scrollerHeight );
		visibleY = rect.Top + scrollPosPixels;
		Float t = ( lineCount - visibleLinesStart ) / visibleLinesCount;
		if ( t <= 1 )
			visibleY += scrollerHeight * ( 1.f - t );
		minimapStartLine = visibleLinesStart - eefloor( scrollPosPixels / lineSpacing );
		minimapStartLine = eemax( 0, eemin( minimapStartLine, lineCount - maxMinmapLines ) );
	}

	Primitives primitives;
	primitives.setForceDraw( false );

	if ( mMinimapConfig.drawBackground ) {
		primitives.setColor( Color( mMinimapBackgroundColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle( rect );
	}

	primitives.setColor( Color( mMinimapVisibleAreaColor ).blendAlpha( mAlpha ) );
	primitives.drawRectangle(
		{ { rect.Left, visibleY }, Sizef( rect.getWidth(), scrollerHeight ) } );
	if ( mMinimapHover || mMinimapDragging ) {
		primitives.setColor( Color( mMinimapHoverColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			{ { rect.Left, visibleY }, Sizef( rect.getWidth(), scrollerHeight ) } );
	}

	Float gutterWidth = PixelDensity::dpToPx( mMinimapConfig.gutterWidth );
	Float lineY = rect.Top;
	Color color = mColorScheme.getSyntaxStyle( "normal" ).color;
	color.a *= 0.5f;
	Float batchWidth = 0;
	Float batchStart = rect.Left;
	Float minimapCutoffX = rect.Left + rect.getWidth();
	std::string batchSyntaxType = "normal";
	Float widthScale = charSpacing / getGlyphWidth();
	auto flushBatch = [&]( const std::string& type ) {
		Color oldColor = color;
		color = mColorScheme.getSyntaxStyle( batchSyntaxType ).color;
		if ( mMinimapConfig.syntaxHighlight && color != Color::Transparent ) {
			color.a *= 0.5f;
		} else {
			color = oldColor;
		}

		if ( batchWidth > 0 ) {
			primitives.setColor( color.blendAlpha( mAlpha ) );
			primitives.drawRectangle( { { batchStart, lineY }, { batchWidth, charHeight } } );
		}

		batchSyntaxType = type;
		batchStart += batchWidth;
		batchWidth = 0;
	};

	int endidx = minimapStartLine + maxMinmapLines;
	endidx = eemin( endidx, lineCount - 1 );

	auto drawWordMatch = [&]( const String& text, const Int64& ln ) {
		size_t pos = 0;
		const String& line( mDoc->line( ln ).getText() );
		if ( line.size() > 300 )
			return;
		primitives.setColor( Color( mMinimapHighlightColor ).blendAlpha( mAlpha ) );

		do {
			pos = line.find( text, pos );
			if ( pos != String::InvalidPos ) {
				Rectf selRect;
				Int64 startCol = pos;
				Int64 endCol = pos + text.size();
				selRect.Top = lineY;
				selRect.Bottom = lineY + charHeight;
				selRect.Left = batchStart + getXOffsetCol( { ln, startCol } ) * widthScale;
				selRect.Right = batchStart + getXOffsetCol( { ln, endCol } ) * widthScale;
				if ( selRect.Left < minimapCutoffX )
					primitives.drawRectangle( selRect );
				pos = endCol;
			} else {
				break;
			}
		} while ( true );
	};

	Float minimapStart = rect.Left + gutterWidth;
	auto drawTextRange = [&]( const TextRange& range, const Int64& ln,
							  const Color& backgroundColor ) {
		if ( !( ln >= range.start().line() && ln <= range.end().line() ) )
			return;

		primitives.setColor( backgroundColor );

		const String& line = mDoc->line( ln ).getText();
		Rectf selRect;
		selRect.Top = lineY;
		selRect.Bottom = lineY + charHeight;
		if ( range.start().line() == ln ) {
			selRect.Left =
				minimapStart + getXOffsetCol( { ln, range.start().column() } ) * widthScale;
			if ( range.end().line() == ln ) {
				selRect.Right =
					minimapStart + getXOffsetCol( { ln, range.end().column() } ) * widthScale;
			} else {
				selRect.Right =
					minimapStart +
					getXOffsetCol( { ln, static_cast<Int64>( line.length() ) } ) * widthScale;
			}
		} else if ( range.end().line() == ln ) {
			selRect.Left = minimapStart + getXOffsetCol( { ln, 0 } ) * widthScale;
			selRect.Right =
				minimapStart + getXOffsetCol( { ln, range.end().column() } ) * widthScale;
		} else {
			selRect.Left = minimapStart + getXOffsetCol( { ln, 0 } ) * widthScale;
			selRect.Right =
				minimapStart +
				getXOffsetCol( { ln, static_cast<Int64>( line.length() ) } ) * widthScale;
		}

		primitives.drawRectangle( selRect );
	};

	auto drawWordRanges = [&]( const TextRanges& ranges ) {
		primitives.setColor( Color( mMinimapHighlightColor ).blendAlpha( mAlpha ) );

		for ( const auto& range : ranges ) {
			if ( !( range.start().line() >= minimapStartLine && range.end().line() <= endidx ) ||
				 !range.inSameLine() )
				continue;

			if ( ranges.isSorted() && range.end().line() > endidx )
				break;

			Rectf selRect;
			selRect.Top = rect.Top + ( range.start().line() - minimapStartLine ) * lineSpacing;
			selRect.Bottom = selRect.Top + charHeight;
			selRect.Left = minimapStart + getXOffsetCol( range.start() ) * widthScale;
			selRect.Right = minimapStart + getXOffsetCol( range.end() ) * widthScale;
			primitives.drawRectangle( selRect );
		}
	};

	String selectionString;

	if ( mDoc->hasSelection() &&
		 mDoc->getSelection().start().line() == mDoc->getSelection().end().line() ) {
		TextRange selection = mDoc->getSelection( true );
		const String& selectionLine = mDoc->line( selection.start().line() ).getText();
		if ( selection.start().column() >= 0 &&
			 selection.start().column() < (Int64)selectionLine.size() &&
			 selection.end().column() >= 0 &&
			 selection.end().column() < (Int64)selectionLine.size() ) {
			String text(
				selectionLine.substr( selection.start().column(),
									  selection.end().column() - selection.start().column() ) );
			if ( !text.empty() )
				selectionString = text;
		}
	}

	if ( !mHighlightWord.isEmpty() )
		drawWordRanges( mHighlightWordCache );

	if ( mMinimapConfig.syntaxHighlight ) {
		for ( int index = minimapStartLine; index <= endidx; index++ ) {
			batchSyntaxType = "normal";
			batchStart = rect.Left + gutterWidth;
			batchWidth = 0;

			if ( mHighlightWord.isEmpty() && !selectionString.empty() )
				drawWordMatch( selectionString, index );

			for ( auto* plugin : mPlugins )
				plugin->minimapDrawBeforeLineText( this, index, { rect.Left, lineY },
												   { rect.getWidth(), charHeight }, charSpacing,
												   gutterWidth );

			const auto& tokens = mDoc->getHighlighter()->getLine( index );
			const auto& text = mDoc->line( index ).getText();
			size_t txtPos = 0;

			for ( const auto& token : tokens ) {
				if ( batchSyntaxType != token.type ) {
					flushBatch( batchSyntaxType );
					batchSyntaxType = token.type;
				}

				size_t pos = txtPos;
				size_t end = pos + token.len <= text.size() ? txtPos + token.len : text.size();

				while ( pos < end ) {
					String::StringBaseType ch = text[pos];
					if ( ch == ' ' || ch == '\n' ) {
						flushBatch( token.type );
						batchStart += charSpacing;
					} else if ( ch == '\t' ) {
						flushBatch( token.type );
						batchStart += charSpacing * mMinimapConfig.tabWidth;
					} else if ( batchStart + batchWidth > minimapCutoffX ) {
						flushBatch( token.type );
						break;
					} else {
						batchWidth += charSpacing;
					}
					pos++;
				};

				txtPos += token.len;
			}

			flushBatch( "normal" );

			for ( auto* plugin : mPlugins )
				plugin->minimapDrawAfterLineText( this, index, { rect.Left, lineY },
												  { rect.getWidth(), charHeight }, charSpacing,
												  gutterWidth );

			if ( mHighlightTextRange.isValid() && mHighlightTextRange.hasSelection() ) {
				drawTextRange( mHighlightTextRange, index,
							   Color( mMinimapSelectionColor ).blendAlpha( mAlpha ) );
			}

			if ( mDoc->hasSelection() ) {
				Color selectionColor( Color( mMinimapSelectionColor ).blendAlpha( mAlpha ) );
				auto selections = mDoc->getSelectionsSorted();
				for ( const auto& sel : selections ) {
					drawTextRange( sel, index, selectionColor );
				}
			}

			lineY = lineY + lineSpacing;
		}
	} else {
		for ( int index = minimapStartLine; index <= endidx; index++ ) {
			batchSyntaxType = "normal";
			batchStart = rect.Left + gutterWidth;
			batchWidth = 0;

			if ( mHighlightWord.isEmpty() && !selectionString.empty() )
				drawWordMatch( selectionString, index );

			const String& text( mDoc->line( index ).getText() );
			for ( size_t i = 0; i < text.size(); ++i ) {
				String::StringBaseType ch = text[i];
				if ( ch == ' ' || ch == '\n' ) {
					flushBatch( "normal" );
					batchStart += charSpacing;
				} else if ( ch == '\t' ) {
					flushBatch( "normal" );
					batchStart += charSpacing * mMinimapConfig.tabWidth;
				} else if ( batchStart + batchWidth > minimapCutoffX ) {
					flushBatch( "normal" );
					break;
				} else {
					batchWidth += charSpacing;
				}
			}
			flushBatch( "normal" );
			lineY = lineY + lineSpacing;
		}
	}

	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		Float selectionY =
			rect.Top +
			( mDoc->getSelectionIndex( i ).start().line() - minimapStartLine ) * lineSpacing;
		primitives.setColor( Color( mMinimapCurrentLineColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle( { { rect.Left, selectionY }, { rect.getWidth(), lineSpacing } } );
	}
	primitives.setForceDraw( true );
}

Vector2f UICodeEditor::getScreenStart() const {
	return Vector2f( eefloor( mScreenPos.x + mPaddingPx.Left ),
					 eefloor( mScreenPos.y + mPaddingPx.Top ) );
}

Float UICodeEditor::getMinimapLineSpacing() const {
	return eemax( 1.f, eefloor( 2 * PixelDensity::getPixelDensity() * mMinimapConfig.scale ) );
}

bool UICodeEditor::isMinimapFileTooLarge() const {
	return mDoc->linesCount() > 1 &&
		   mDoc->linesCount() >
			   eefloor( getMinimapRect( getScreenStart() ).getHeight() / getMinimapLineSpacing() );
}

const Time& UICodeEditor::getCursorBlinkTime() const {
	return mBlinkTime;
}

void UICodeEditor::setCursorBlinkTime( const Time& blinkTime ) {
	mBlinkTime = blinkTime;
	if ( mBlinkTime == Time::Zero && !mCursorVisible && hasFocus() &&
		 getUISceneNode()->getWindow()->hasFocus() ) {
		resetCursor();
	}
}

bool UICodeEditor::getAutoCloseXMLTags() const {
	return mAutoCloseXMLTags;
}

void UICodeEditor::setAutoCloseXMLTags( bool autoCloseXMLTags ) {
	mAutoCloseXMLTags = autoCloseXMLTags;
}

bool UICodeEditor::checkAutoCloseXMLTag( const String& text ) {
	if ( !mAutoCloseXMLTags || text.empty() || text.size() > 1 || text[0] != '>' ||
		 mDoc->getSelection().hasSelection() )
		return false;
	TextPosition start( mDoc->getSelection().start() );
	if ( start.line() >= (Int64)mDoc->linesCount() )
		return false;
	const auto& line = mDoc->line( start.line() ).getText();
	if ( start.column() >= (Int64)line.size() )
		return false;
	if ( line[start.column() - 1] != '>' || ( line.size() > 2 && line[start.column() - 2] == '/' ) )
		return false;
	const SyntaxDefinition& definition =
		mDoc->getHighlighter()->getSyntaxDefinitionFromTextPosition( start );
	if ( !definition.getAutoCloseXMLTags() )
		return false;
	size_t foundOpenPos = line.find_last_of( "<", start.column() - 1 );
	if ( foundOpenPos == String::InvalidPos || start.column() - foundOpenPos < 1 )
		return false;
	std::string tag( line.substr( foundOpenPos, start.column() - foundOpenPos ).toUtf8() );
	LuaPattern pattern( "<([%w_%-]+).*>" );
	auto match = pattern.gmatch( tag );
	if ( match.matches() ) {
		std::string tagName( match.group( 1 ) );
		mDoc->textInput( String( "</" + tagName + ">" ) );
		mDoc->setSelection( start );
		return true;
	}
	return false;
}

Int64 UICodeEditor::getCurrentColumnCount() const {
	Int64 curLine = mDoc->getSelection().start().line();
	Int64 sel = mDoc->getSelection().start().column();
	Int64 count = 0;
	for ( Int64 i = 0; i < sel; i++ )
		count += mDoc->line( curLine ).getText()[i] == '\t' ? mTabWidth : 1;
	return count;
}

bool UICodeEditor::getFindReplaceEnabled() const {
	return mFindReplaceEnabled;
}

void UICodeEditor::setFindReplaceEnabled( bool findReplaceEnabled ) {
	mFindReplaceEnabled = findReplaceEnabled;
}

const Vector2f& UICodeEditor::getScroll() const {
	return mScroll;
}

Text& UICodeEditor::getLineText( const Int64& lineNumber ) const {
	auto it = mTextCache.find( lineNumber );
	if ( it == mTextCache.end() || it->second.hash != mDoc->line( lineNumber ).getHash() ) {
		Float fontSize = PixelDensity::pxToDp( getCharacterSize() );
		Text txt( "", mFont, fontSize );
		txt.setTabWidth( mTabWidth );
		txt.setStyleConfig( mFontStyleConfig );
		txt.setString( mDoc->line( lineNumber ).getText() );
		mTextCache[lineNumber] = { std::move( txt ), mDoc->line( lineNumber ).getHash() };
		return mTextCache[lineNumber].text;
	}
	return it->second.text;
}

void UICodeEditor::updateLineCache( const Int64& lineIndex ) {
	if ( lineIndex >= 0 && lineIndex < (Int64)mDoc->linesCount() ) {
		TextDocumentLine& line = mDoc->line( lineIndex );
		auto& cacheLine = mTextCache[lineIndex];
		cacheLine.text.setStyleConfig( mFontStyleConfig );
		cacheLine.text.setString( line.getText() );
		cacheLine.hash = line.getHash();
	}
}

void UICodeEditor::invalidateLinesCache() {
	if ( mFont && !mFont->isMonospace() ) {
		mTextCache.clear();
		invalidateDraw();
	}
}

bool UICodeEditor::stopMinimapDragging( const Vector2f& mousePos ) {
	if ( mMinimapDragging ) {
		mMinimapDragging = false;
		getEventDispatcher()->setNodeDragging( NULL );
		mVScrollBar->setEnabled( true );
		getUISceneNode()->setCursor( Cursor::Arrow );
		updateMipmapHover( mousePos );
		return true;
	}
	return false;
}

}} // namespace EE::UI
