#include <algorithm>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
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
		{ { KEY_PAGEUP, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "jump-lines-up" },
		{ { KEY_UP, KEYMOD_SHIFT }, "select-to-previous-line" },
		{ { KEY_UP, 0 }, "move-to-previous-line" },
		{ { KEY_DOWN, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "move-lines-down" },
		{ { KEY_DOWN, KeyMod::getDefaultModifier() }, "move-scroll-down" },
		{ { KEY_PAGEDOWN, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "jump-lines-down" },
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
		{ { KEY_D, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-all-words" },
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
	mLineWrapping( mDoc, mFontStyleConfig, {} ),
	mBlinkTime( Seconds( 0.5f ) ),
	mTabWidth( 4 ),
	mMouseWheelScroll( 50 ),
	mFontSize( mFontStyleConfig.getFontCharacterSize() ),
	mLineNumberPaddingLeft( 8 ),
	mLineNumberPaddingRight( 8 ),
	mKeyBindings( getUISceneNode()->getWindow()->getInput() ),
	mFindLongestLineWidthUpdateFrequency( Seconds( 1 ) ),
	mPreviewColor( Color::Transparent ) {
	mFlags |= UI_TAB_STOP | UI_OWNS_CHILDS_POSITION | UI_SCROLLABLE;
	setTextSelection( true );
	setColorScheme( SyntaxColorScheme::getDefault() );
	mVScrollBar = UIScrollBar::NewVertical();
	mVScrollBar->setParent( this );
	mVScrollBar->addEventListener( Event::OnSizeChange,
								   [this]( const Event* ) { updateScrollBar(); } );
	mVScrollBar->addEventListener( Event::OnValueChange, [this]( const Event* ) {
		setScrollY( mVScrollBar->getValue() * getMaxScroll().y, false );
	} );

	mHScrollBar = UIScrollBar::NewHorizontal();
	mHScrollBar->setParent( this );
	mHScrollBar->addEventListener( Event::OnSizeChange,
								   [this]( const Event* ) { updateScrollBar(); } );
	mHScrollBar->addEventListener( Event::OnValueChange, [this]( const Event* ) {
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
	if ( getUISceneNode()->hasThreadPool() ) {
		Uint64 tag = reinterpret_cast<Uint64>( this );
		getUISceneNode()->getThreadPool()->removeWithTag( tag );
	}

	if ( mCurrentMenu ) {
		mCurrentMenu->clearEventListener();
		mCurrentMenu = nullptr;
	}

	for ( auto& plugin : mPlugins )
		plugin->onUnregister( this );

	// Remember to stop all the async find jobs
	mDoc->stopActiveFindAll();

	// TODO: Use a condition variable to wait the thread pool to finish
	// Wait to end all the async find jobs
	while ( mHighlightWordProcessing )
		Sys::sleep( Milliseconds( 0.1 ) );

	mLineWrapping.setDocument( nullptr );
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

	if ( mLineWrapping.isPendingReconstruction() )
		mLineWrapping.reconstructBreaks();

	Color col;
	auto lineRange = getVisibleLineRange();
	auto visualLineRange = getVisibleLineRange( true );
	Float charSize = getCharacterSize();
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
		for ( const auto& sel : mDoc->getSelections() ) {
			primitives.setColor( Color( mCurrentLineBackgroundColor ).blendAlpha( mAlpha ) );
			Float height = 1;
			if ( mLineWrapping.isWrappedLine( sel.start().line() ) )
				height = mLineWrapping.getVisualLine( sel.start().line() ).visualLines.size();
			primitives.drawRectangle( Rectf(
				Vector2f( startScroll.x + mScroll.x,
						  startScroll.y +
							  mLineWrapping.toWrappedIndex( sel.start().line() ) * lineHeight ),
				Sizef( mSize.getWidth(), lineHeight * height ) ) );
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
					   mColorScheme.getEditorSyntaxStyle( "selection_region"_sst ).color );
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

	if ( !mHighlightWord.isEmpty() ) {
		Lock l( mHighlightWordCacheMutex );
		drawWordRanges( mHighlightWordCache, lineRange, startScroll, lineHeight, true );
	}

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
			{ startScroll.x,
			  static_cast<float>( startScroll.y +
								  lineHeight * (double)mLineWrapping.toWrappedIndex( i ) ) } );

		for ( auto& plugin : mPlugins )
			plugin->drawBeforeLineText( this, i, curScroll, charSize, lineHeight );

		drawLineText( i, curScroll, charSize, lineHeight, visualLineRange );

		for ( auto& plugin : mPlugins )
			plugin->drawAfterLineText( this, i, curScroll, charSize, lineHeight );

		if ( mPluginsGutterSpace > 0 ) {
			Float curGutterPos = 0.f;
			for ( auto& plugin : mPluginGutterSpaces ) {
				for ( unsigned long gi = lineRange.first; gi <= lineRange.second; gi++ ) {
					plugin.plugin->drawGutter( this, gi,
											   { screenStart.x + curGutterPos, curScroll.y },
											   lineHeight, plugin.space, charSize );
				}
				curGutterPos += plugin.space;
			}
		}
	}

	if ( hasFocus() && getUISceneNode()->getWindow()->getIME().isEditing() ) {
		auto offset = getTextPositionOffset( cursor, lineHeight );
		Vector2f cursorPos( startScroll.x + offset.x, startScroll.y + offset.y );
		FontStyleConfig config( mFontStyleConfig );
		config.FontColor = mFontStyleConfig.getFontSelectedColor();
		getUISceneNode()->getWindow()->getIME().draw( cursorPos, getFontHeight(), config,
													  Color( mCaretColor ).blendAlpha( mAlpha ) );
	} else {
		for ( const auto& sel : mDoc->getSelections() )
			drawCursor( startScroll, lineHeight, sel.start() );
	}

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

	if ( mLocked && mDisplayLockedIcon )
		drawLockedIcon( start );

	for ( auto& plugin : mPlugins )
		plugin->postDraw( this, startScroll, lineHeight, cursor );
}

void UICodeEditor::scheduledUpdate( const Time& ) {
	if ( mLastActivity.getElapsedTime() > Seconds( 60 ) ) {
		if ( !mCursorVisible ) {
			mCursorVisible = true;
			invalidateDraw();
		}
	} else if ( hasFocus() && getUISceneNode()->getWindow()->hasFocus() ) {
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
		onDocumentLoaded();
	}
	return ret;
}

bool UICodeEditor::loadAsyncFromFile(
	const std::string& path, std::shared_ptr<ThreadPool> pool,
	std::function<void( std::shared_ptr<TextDocument>, bool )> onLoaded ) {
	bool wasLocked = isLocked();
	if ( !wasLocked )
		setLocked( true );
	bool ret = mDoc->loadAsyncFromFile(
		path, pool, [this, onLoaded, wasLocked]( TextDocument*, bool success ) {
			if ( !success ) {
				runOnMainThread( [this, onLoaded, wasLocked, success] {
					if ( !wasLocked )
						setLocked( false );
					if ( onLoaded )
						onLoaded( mDoc, success );
				} );
				return;
			}
			if ( mMinimapEnabled && getUISceneNode()->hasThreadPool() ) {
				mDoc->getHighlighter()->tokenizeAsync( getUISceneNode()->getThreadPool(), [this] {
					runOnMainThread( [this] { invalidateDraw(); } );
				} );
			}

			if ( mLineWrapping.isWrapEnabled() )
				mLineWrapping.setPendingReconstruction( true );

			runOnMainThread( [this, onLoaded, wasLocked, success] {
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
			if ( mMinimapEnabled && getUISceneNode()->hasThreadPool() )
				mDoc->getHighlighter()->tokenizeAsync( getUISceneNode()->getThreadPool(), [this] {
					runOnMainThread( [this] { invalidateDraw(); } );
				} );
			runOnMainThread( [this, success, onLoaded, wasLocked] {
				if ( !wasLocked )
					setLocked( false );
				onDocumentLoaded();
				if ( onLoaded )
					onLoaded( mDoc, success );
			} );
		},
		[this]( const Http&, const Http::Request&, const Http::Response&,
				const Http::Request::Status& status, size_t /*totalBytes*/,
				size_t /*currentBytes*/ ) {
			if ( status == Http::Request::ContentReceived ) {
				runOnMainThread( [this] { invalidateDraw(); } );
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
	udpateGlyphWidth();
	mLineWrapping.setFontStyle( mFontStyleConfig );
	invalidateDraw();
}

void UICodeEditor::onFontStyleChanged() {
	udpateGlyphWidth();
	mLineWrapping.setFontStyle( mFontStyleConfig );
	invalidateDraw();
}

void UICodeEditor::onDocumentLoaded( TextDocument* ) {
	if ( mInvalidateOnLoaded ) {
		onDocumentLoaded();
		mInvalidateOnLoaded = false;
	}
}

void UICodeEditor::invalidateLineWrapMaxWidth( bool force ) {
	switch ( mLineWrapType ) {
		case LineWrapType::Viewport:
			mLineWrapping.setMaxWidth( getViewportWidth(), force );
			break;
		case LineWrapType::LineBreakingColumn:
			mLineWrapping.setMaxWidth( getGlyphWidth() * mLineBreakingColumn, force );
			break;
	}
}

void UICodeEditor::onDocumentReloaded( TextDocument* ) {
	DocEvent event( this, mDoc.get(), Event::OnDocumentReloaded );
	sendEvent( &event );
	invalidateDraw();
	invalidateLongestLineWidth();
	invalidateLineWrapMaxWidth( true );
}

void UICodeEditor::onDocumentLoaded() {
	DocEvent event( this, mDoc.get(), Event::OnDocumentLoaded );
	sendEvent( &event );
	invalidateEditor();
	invalidateDraw();
	invalidateLongestLineWidth();
	invalidateLineWrapMaxWidth( true );
}

void UICodeEditor::onDocumentReset( TextDocument* ) {
	DocEvent event( this, mDoc.get(), Event::OnDocumentReset );
	sendEvent( &event );
	mLineWrapping.clear();
	invalidateEditor();
	invalidateDraw();
	invalidateLongestLineWidth();
	invalidateLineWrapMaxWidth( true );
}

void UICodeEditor::onDocumentChanged() {
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
	mUseDefaultStyle = true;
}

Float UICodeEditor::getViewportWidth( const bool& forceVScroll ) const {
	Float vScrollWidth =
		mVScrollBar->isVisible() || forceVScroll ? mVScrollBar->getPixelsSize().getWidth() : 0.f;
	if ( mMinimapEnabled )
		vScrollWidth += getMinimapWidth();
	Float viewWidth = eefloor( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right -
							   getGutterWidth() - vScrollWidth );
	return eemax( 0.f, viewWidth );
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

UICodeEditor* UICodeEditor::setFontSize( const Float& size ) {
	if ( mFontStyleConfig.CharacterSize != size ) {
		mFontStyleConfig.CharacterSize =
			eeabs( size - (int)size ) == 0.5f || (int)size == size ? size : eefloor( size );
		mFontSize = mFontStyleConfig.CharacterSize;
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
	if ( mTabWidth != tabWidth ) {
		mTabWidth = tabWidth;
		auto config = mLineWrapping.getConfig();
		config.tabWidth = tabWidth;
		mLineWrapping.setConfig( config );
	}
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
	return eemax<size_t>( 2UL, Math::countDigits( (Int64)mDoc->linesCount() ) );
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
	setBackgroundColor( mColorScheme.getEditorColor( "background"_sst ) );
	setFontColor( mColorScheme.getEditorColor( "text"_sst ) );
	mFontStyleConfig.setFontSelectionBackColor( mColorScheme.getEditorColor( "selection"_sst ) );
	mLineNumberFontColor = mColorScheme.getEditorColor( "line_number"_sst );
	mLineNumberActiveFontColor = mColorScheme.getEditorColor( "line_number2"_sst );
	mLineNumberBackgroundColor = mColorScheme.getEditorColor( "gutter_background"_sst );
	mCurrentLineBackgroundColor = mColorScheme.getEditorColor( "line_highlight"_sst );
	mCaretColor = mColorScheme.getEditorColor( "caret"_sst );
	mWhitespaceColor = mColorScheme.getEditorColor( "whitespace"_sst );
	mLineBreakColumnColor = mColorScheme.getEditorColor( "line_break_column"_sst );
	mMatchingBracketColor = mColorScheme.getEditorColor( "matching_bracket"_sst );
	mSelectionMatchColor = mColorScheme.getEditorColor( "matching_selection"_sst );
	mMinimapBackgroundColor = mColorScheme.getEditorColor( "minimap_background"_sst );
	mMinimapVisibleAreaColor = mColorScheme.getEditorColor( "minimap_visible_area"_sst );
	mMinimapCurrentLineColor = mColorScheme.getEditorColor( "minimap_current_line"_sst );
	mMinimapHoverColor = mColorScheme.getEditorColor( "minimap_hover"_sst );
	mMinimapHighlightColor = mColorScheme.getEditorColor( "minimap_highlight"_sst );
	mMinimapSelectionColor = mColorScheme.getEditorColor( "minimap_selection"_sst );
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
		mLineWrapping.setDocument( nullptr );
		if ( mDoc.use_count() == 1 )
			onDocumentClosed( mDoc.get() );
		mDoc = doc;
		mDoc->registerClient( this );
		mLineWrapping.setDocument( doc );
		onDocumentChanged();
		if ( mDoc->isLoading() ) {
			mInvalidateOnLoaded = true;
		} else {
			invalidateEditor();
			invalidateLongestLineWidth();
			invalidateDraw();
			mLineWrapping.setDocument( mDoc );
		}
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

void UICodeEditor::setLineWrapMode( LineWrapMode mode ) {
	auto prevMode = mLineWrapping.getConfig().mode;
	mLineWrapping.setLineWrapMode( mode );
	if ( prevMode != mode && LineWrapMode::NoWrap == mode ) {
		invalidateLongestLineWidth();
	}
}

LineWrapType UICodeEditor::getLineWrapType() const {
	return mLineWrapType;
}

void UICodeEditor::setLineWrapType( LineWrapType lineWrapType ) {
	if ( mLineWrapType != lineWrapType ) {
		mLineWrapType = lineWrapType;
		invalidateLineWrapMaxWidth( true );
	}
}

void UICodeEditor::setLineWrapKeepIndentation( bool keep ) {
	auto config = mLineWrapping.getConfig();
	config.keepIndentation = keep;
	mLineWrapping.setConfig( config );
}

Uint32 UICodeEditor::onFocus( NodeFocusReason reason ) {
	if ( !mLocked ) {
		mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();
		resetCursor();
		getUISceneNode()->getWindow()->startTextInput();
		mDoc->setActiveClient( this );
		updateIMELocation();
	}
	for ( auto& plugin : mPlugins )
		plugin->onFocus( this );
	mLastActivity.restart();
	return UIWidget::onFocus( reason );
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
	mLastActivity.restart();
	return UIWidget::onFocusLoss();
}

Uint32 UICodeEditor::onTextInput( const TextInputEvent& event ) {
	mLastActivity.restart();

	if ( mLocked || NULL == mFont )
		return 0;
	Input* input = getUISceneNode()->getWindow()->getInput();

	if ( ( input->isLeftAltPressed() && !event.getText().empty() && event.getText()[0] == '\t' ) ||
		 ( input->isLeftControlPressed() && !input->isLeftAltPressed() &&
		   !input->isAltGrPressed() ) ||
		 input->isMetaPressed() || ( input->isLeftAltPressed() && !input->isLeftControlPressed() ) )
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

void UICodeEditor::updateIMELocation() {
	if ( mDoc->getActiveClient() != this || !Engine::isRunninMainThread() )
		return;
	Rectf r( getScreenPosition( mDoc->getSelection( true ).start() ) );
	getUISceneNode()->getWindow()->getIME().setLocation( r.asInt() );
}

void UICodeEditor::drawLockedIcon( const Vector2f start ) {
	if ( mFileLockIcon == nullptr && !mFileLockIconName.empty() )
		mFileLockIcon = getUISceneNode()->findIcon( mFileLockIconName );
	if ( mFileLockIcon == nullptr )
		return;

	Drawable* fileLockIcon = mFileLockIcon->getSize( PixelDensity::dpToPxI( 16 ) );
	if ( fileLockIcon == nullptr )
		return;

	Float w = fileLockIcon->getPixelsSize().getWidth();
	Float posX =
		mMinimapEnabled
			? getMinimapRect( getScreenStart() ).Left - w
			: ( start.x + mSize.getWidth() -
				( mVScrollBar->isVisible() ? mVScrollBar->getPixelsSize().getWidth() : 0 ) ) -
				  mPadding.Right - w;
	Color col( fileLockIcon->getColor() );
	fileLockIcon->setColor( Color( mFontStyleConfig.getFontColor() ).blendAlpha( mAlpha ) );
	Float margin = PixelDensity::dpToPxI( 4 );
	fileLockIcon->draw( { posX - margin, start.y + margin } );
	fileLockIcon->setColor( col );
}

size_t UICodeEditor::getTotalVisibleLines() const {
	return mLineWrapping.getTotalLines();
}

Uint32 UICodeEditor::onTextEditing( const TextEditingEvent& event ) {
	UIWidget::onTextEditing( event );
	mLastActivity.restart();
	mDoc->imeTextEditing( event.getText() );
	updateIMELocation();
	invalidateDraw();
	return 1;
}

Uint32 UICodeEditor::onKeyDown( const KeyEvent& event ) {
	if ( getUISceneNode()->getWindow()->getIME().isEditing() )
		return 0;

	mLastActivity.restart();

	if ( NULL == mFont || mUISceneNode->getUIEventDispatcher()->justGainedFocus() )
		return 0;

	for ( auto& plugin : mPlugins )
		if ( plugin->onKeyDown( this, event ) )
			return 1;

	std::string cmd = mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( !cmd.empty() ) {
		// Allow copy selection on locked mode
		if ( !mLocked || mUnlockedCmd.find( cmd ) != mUnlockedCmd.end() ) {
			mDoc->execute( cmd, this );
			mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();
			return 1;
		}
	}
	return 0;
}

Uint32 UICodeEditor::onKeyUp( const KeyEvent& event ) {
	mLastActivity.restart();
	for ( auto& plugin : mPlugins )
		if ( plugin->onKeyUp( this, event ) )
			return 1;
	if ( mHandShown && !getUISceneNode()->getWindow()->getInput()->isKeyModPressed() )
		resetLinkOver();
	return UIWidget::onKeyUp( event );
}

TextPosition UICodeEditor::resolveScreenPosition( const Vector2f& position, bool clamp ) const {
	Vector2f localPos( convertToNodeSpace( position ) );
	localPos += mScroll;
	localPos.x -= mPaddingPx.Left + getGutterWidth();
	localPos.y -= mPaddingPx.Top;
	localPos.y -= getPluginsTopSpace();
	Int64 visibleLineIndex = (Int64)eefloor( localPos.y / getLineHeight() );
	if ( clamp ) {
		visibleLineIndex =
			eeclamp<Int64>( visibleLineIndex, 0, (Int64)( getTotalVisibleLines() - 1 ) );
	}
	return TextPosition( mLineWrapping.getDocumentLine( visibleLineIndex ).line(),
						 getColFromXOffset( visibleLineIndex, localPos.x ) );
}

Rectf UICodeEditor::getVisibleScrollArea() const {
	return { mScroll, getViewportDimensions() };
}

Sizef UICodeEditor::getViewportDimensions() const {
	Float h = mSize.getHeight() - getPluginsTopSpace() - mPaddingPx.Top;
	return { getViewportWidth(), h };
}

Rectf UICodeEditor::getScreenPosition( const TextPosition& position ) const {
	Float lineHeight = getLineHeight();
	Vector2f screenStart( getScreenStart() );
	Vector2f start( screenStart.x + getGutterWidth(), screenStart.y );
	Vector2f startScroll( start - mScroll );
	auto offset = getTextPositionOffsetSanitized( position, lineHeight );
	return { { startScroll.x + offset.x, startScroll.y + offset.y },
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
				  vplc.y > getTotalVisibleLines() - 1
					  ? 0.f
					  : eefloor( getTotalVisibleLines() - getViewPortLineCount().y ) *
							getLineHeight() );
}

UIMenuItem* UICodeEditor::menuAdd( UIPopUpMenu* menu, const String& translateString,
								   const std::string& icon, const std::string& cmd ) {
	UIMenuItem* menuItem =
		menu->add( translateString, findIcon( icon ), mKeyBindings.getCommandKeybindString( cmd ) );
	menuItem->setId( cmd );
	return menuItem;
}

void UICodeEditor::createDefaultContextMenuOptions( UIPopUpMenu* menu ) {
	if ( !mCreateDefaultContextMenuOptions )
		return;

	if ( !mLocked ) {
		menuAdd( menu, i18n( "uicodeeditor_undo", "Undo" ), "undo", "undo" )
			->setEnabled( mDoc->hasUndo() );
		menuAdd( menu, i18n( "uicodeeditor_redo", "Redo" ), "redo", "redo" )
			->setEnabled( mDoc->hasRedo() );
		menu->addSeparator();

		menuAdd( menu, i18n( "uicodeeditor_cut", "Cut" ), "cut", "cut" )
			->setEnabled( mDoc->hasSelection() );
	}

	menuAdd( menu, i18n( "uicodeeditor_copy", "Copy" ), "copy", "copy" )
		->setEnabled( mDoc->hasSelection() );

	if ( !mLocked ) {
		menuAdd( menu, i18n( "uicodeeditor_paste", "Paste" ), "paste", "paste" );
		menuAdd( menu, i18n( "uicodeeditor_delete", "Delete" ), "delete-text",
				 "delete-to-next-char" );
	}

	menu->addSeparator();
	menuAdd( menu, i18n( "uicodeeditor_select_all", "Select All" ), "select-all", "select-all" );

	if ( mDoc->hasFilepath() ) {
		menu->addSeparator();

		menuAdd(
			menu,
			i18n( "uicodeeditor_open_containing_folder_ellipsis", "Open Containing Folder..." ),
			"folder-open", "open-containing-folder" );

		menuAdd( menu,
				 i18n( "uicodeeditor_copy_containing_folder_path_ellipsis",
					   "Copy Containing Folder Path..." ),
				 "copy", "copy-containing-folder-path" );

		menuAdd( menu, i18n( "uicodeeditor_copy_file_path", "Copy File Path" ), "copy",
				 "copy-file-path" );

		menuAdd( menu,
				 i18n( "uicodeeditor_copy_file_path_and_position", "Copy File Path and Position" ),
				 "copy", "copy-file-path-and-position" );
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
	menu->addClass( "uicodeeditor_menu" );

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
	menu->setCloseSubMenusOnClose( true );

	UICodeEditor* editor = this;
	const auto registerMenu = [editor, this]( UIMenu* menu ) {
		menu->addEventListener( Event::OnItemClicked, [this, menu, editor]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
			std::string txt( item->getId() );
			mDoc->execute( txt, editor );
			menu->hide();
		} );
	};
	registerMenu( menu );
	auto subMenus = menu->findAllByType<UIMenuSubMenu>( UI_TYPE_MENUSUBMENU );
	for ( auto* subMenu : subMenus )
		registerMenu( subMenu->getSubMenu() );

	Vector2f pos( position.asFloat() );
	runOnMainThread( [this, menu, pos]() {
		Vector2f npos( pos );
		menu->nodeToWorldTranslation( npos );
		UIMenu::findBestMenuPos( npos, menu );
		menu->setPixelsPosition( npos );
		menu->show();
		mCurrentMenu = menu;
	} );
	menu->addEventListener( Event::OnMenuHide, [this]( const Event* ) {
		if ( !isClosing() )
			setFocus();
	} );
	menu->addEventListener( Event::OnClose, [this]( const Event* ) { mCurrentMenu = nullptr; } );
	return true;
}

Int64 UICodeEditor::calculateMinimapClickedLine( const Vector2i& position ) {
	auto lineRange = getVisibleLineRange( true );
	Vector2f start( getScreenStart() );
	Float lineSpacing = getMinimapLineSpacing();
	Rectf rect( getMinimapRect( start ) );
	Int64 visibleLinesCount = ( lineRange.second - lineRange.first );
	Int64 visibleLinesStart = lineRange.first;
	Float scrollerHeight = visibleLinesCount * lineSpacing;
	size_t visualLineCount = getTotalVisibleLines();
	Int64 maxMinmapLines = eefloor( rect.getHeight() / lineSpacing );
	Int64 minimapStartLine = 0;
	if ( isMinimapFileTooLarge() ) {
		Float scrollPos =
			( visibleLinesStart - 1 ) / (Float)( visualLineCount - visibleLinesCount - 1 );
		scrollPos = eeclamp( scrollPos, 0.f, 1.f );
		Float scrollPosPixels = scrollPos * ( rect.getHeight() - scrollerHeight );
		minimapStartLine = visibleLinesStart - eefloor( scrollPosPixels / lineSpacing );
		minimapStartLine =
			eemax( (Int64)0, eemin( minimapStartLine, (Int64)visualLineCount - maxMinmapLines ) );
	}
	Float dy = position.y - rect.Top;
	Int64 ret = minimapStartLine + eefloor( dy / lineSpacing );
	return eeclamp( ret, (Int64)0, (Int64)visualLineCount );
}

Uint32 UICodeEditor::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	mLastActivity.restart();
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
				scrollToVisualIndex( calculateMinimapClickedLine( position ), false, true );
				return 1;
			}
			mMouseDown = true;
			mMinimapScrollOffset =
				calculateMinimapClickedLine( position ) - getVisibleLineRange( true ).first;
			mMinimapDragging = true;
			getEventDispatcher()->setNodeDragging( this );
			mVScrollBar->setEnabled( false );
			getUISceneNode()->setCursor( Cursor::Arrow );
			return 1;
		} else if ( mMinimapDragging ) {
			if ( ( flags & EE_BUTTON_LMASK ) && mMouseDown &&
				 rect.contains( position.asFloat() ) ) {
				scrollToVisualIndex( calculateMinimapClickedLine( position ) - mMinimapScrollOffset,
									 false, true );
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
			} else if ( mLastDoubleClick.getElapsedTime() < Milliseconds( 300.f ) ) {
				mDoc->selectLine();
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
	auto lineRange = getVisibleLineRange( true );
	int visibleLinesCount = ( lineRange.second - lineRange.first );
	int visibleLinesStart = lineRange.first;
	Float scrollerHeight = visibleLinesCount * lineSpacing;
	int lineCount = getTotalVisibleLines();
	Float visibleY = rect.Top + visibleLinesStart * lineSpacing;

	if ( isMinimapFileTooLarge() && visibleLinesCount ) {
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
	mLastActivity.restart();
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseMove( this, position, flags ) )
			return UIWidget::onMouseMove( position, flags );

	bool minimapHover = false;
	if ( mMinimapEnabled ) {
		updateMipmapHover( position.asFloat() );
		if ( mMinimapDragging && ( flags & EE_BUTTON_LMASK ) ) {
			scrollToVisualIndex( calculateMinimapClickedLine( position ) - mMinimapScrollOffset,
								 false, true );
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
	mLastActivity.restart();
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

	Input* input = getUISceneNode()->getWindow()->getInput();
	if ( flags & EE_BUTTON_WDMASK ) {
		if ( getUISceneNode()->getWindow()->getInput()->isKeyModPressed() ) {
			mDoc->execute( "font-size-shrink" );
		} else if ( input->isModState( KEYMOD_SHIFT ) ) {
			setScrollX( mScroll.x + PixelDensity::dpToPx( mMouseWheelScroll ) );
		} else {
			setScrollY( mScroll.y + PixelDensity::dpToPx( mMouseWheelScroll ) );
		}
		invalidateDraw();
	} else if ( flags & EE_BUTTON_WUMASK ) {
		if ( getUISceneNode()->getWindow()->getInput()->isKeyModPressed() ) {
			mDoc->execute( "font-size-grow" );
		} else if ( input->isModState( KEYMOD_SHIFT ) ) {
			setScrollX( mScroll.x - PixelDensity::dpToPx( mMouseWheelScroll ) );
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
	mLastActivity.restart();
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseClick( this, position, flags ) )
			return UIWidget::onMouseClick( position, flags );

	if ( mMinimapEnabled ) {
		Rectf rect( getMinimapRect( getScreenStart() ) );
		if ( ( flags & EE_BUTTON_LMASK ) && rect.contains( position.asFloat() ) )
			return 1;
	}

	if ( ( flags & EE_BUTTON_LMASK ) &&
		 getUISceneNode()->getWindow()->getInput()->isKeyModPressed() ) {
		String link( checkMouseOverLink( position ) );
		if ( !link.empty() ) {
			Engine::instance()->openURI( link.toUtf8() );
			resetLinkOver();
		}
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
	mLastActivity.restart();
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
	mLastActivity.restart();
	for ( auto& plugin : mPlugins )
		if ( plugin->onMouseOver( this, position, flags ) )
			return UIWidget::onMouseOver( position, flags );
	if ( getEventDispatcher()->getMouseOverNode() == this )
		getUISceneNode()->setCursor( !mLocked ? Cursor::IBeam : Cursor::Arrow );
	return UIWidget::onMouseOver( position, flags );
}

Uint32 UICodeEditor::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	mLastActivity.restart();
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
			colorPicker = UIColorPicker::NewModal( this, [this]( Color color ) {
				mDoc->replaceSelection( color.toHexString( false ) );
			} );
			colorPicker->setColor( Color( '#' + text ) );
		} else if ( isRgba || isRgb ) {
			TextPosition position = mDoc->getMatchingBracket(
				{ range.start().line(), static_cast<Int64>( range.end().column() ) }, '(', ')',
				TextDocument::MatchDirection::Forward );
			if ( position.isValid() ) {
				mDoc->setSelection( { position.line(), position.column() + 1 }, range.start() );
				colorPicker = UIColorPicker::NewModal( this, [this, isRgba]( Color color ) {
					mDoc->replaceSelection( isRgba || color.a != 255 ? color.toRgbaString()
																	 : color.toRgbString() );
				} );
				colorPicker->setColor( Color::fromString( mDoc->getSelectedText() ) );
			}
		}
		if ( colorPicker )
			colorPicker->getUIWindow()->addEventListener(
				Event::OnWindowClose, [this]( const Event* ) {
					if ( !SceneManager::instance()->isShuttingDown() )
						setFocus();
				} );
	}
}

Vector2f UICodeEditor::getRelativeScreenPosition( const TextPosition& pos ) {
	Float gutterWidth = getGutterWidth();
	Vector2f start( gutterWidth, getPluginsTopSpace() );
	Vector2f startScroll( start - mScroll );
	auto offset = getTextPositionOffset( pos );
	return { startScroll.x + offset.x, startScroll.y + offset.y + getLineOffset() };
}

bool UICodeEditor::getShowLinesRelativePosition() const {
	return mShowLinesRelativePosition;
}

void UICodeEditor::showLinesRelativePosition( bool showLinesRelativePosition ) {
	mShowLinesRelativePosition = showLinesRelativePosition;
}

UIScrollBar* UICodeEditor::getVScrollBar() const {
	return mVScrollBar;
}

UIScrollBar* UICodeEditor::getHScrollBar() const {
	return mHScrollBar;
}

void UICodeEditor::drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							   const TextPosition& cursor ) {
	if ( mCursorVisible && !mLocked && isTextSelectionEnabled() ) {
		auto offset = getTextPositionOffset( cursor, lineHeight );
		Vector2f cursorPos( startScroll.x + offset.x, startScroll.y + offset.y );
		Primitives primitives;
		primitives.setColor( Color( mCaretColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 2 ), getFontHeight() ) ) );
	}
}

void UICodeEditor::onSizeChange() {
	UIWidget::onSizeChange();
	invalidateEditor( false );
	invalidateLineWrapMaxWidth( false );
	if ( mLineWrapping.isWrapEnabled() )
		invalidateLongestLineWidth();
}

void UICodeEditor::onPaddingChange() {
	UIWidget::onPaddingChange();
	invalidateEditor( false );
	invalidateLineWrapMaxWidth( false );
	if ( mLineWrapping.isWrapEnabled() )
		invalidateLongestLineWidth();
}

std::pair<size_t, Float> UICodeEditor::findLongestLineInRange( const TextRange& range ) {
	std::pair<size_t, Float> curRange{ mLongestLineIndex, mLongestLineWidth };
	if ( mHorizontalScrollBarEnabled ) {
		Float longestLineWidth = 0;
		for ( Int64 lineIndex = range.start().line(); lineIndex <= range.end().line();
			  lineIndex++ ) {
			Float lineWidth = getLineWidth( lineIndex );
			if ( lineWidth > longestLineWidth ) {
				curRange.first = lineIndex;
				curRange.second = lineWidth;
				longestLineWidth = lineWidth;
			}
		}
	}
	return curRange;
}

void UICodeEditor::findLongestLine() {
	if ( mHorizontalScrollBarEnabled ) {
		auto range = findLongestLineInRange( mDoc->getDocRange() );
		mLongestLineIndex = range.first;
		mLongestLineWidth = range.second;
	}
}

Float UICodeEditor::getLineWidth( const Int64& docLine ) {
	if ( docLine >= (Int64)mDoc->linesCount() )
		return 0;
	if ( mLineWrapping.isWrappedLine( docLine ) ) {
		auto vline = mLineWrapping.getVisualLine( docLine );
		auto& line = mDoc->line( docLine ).getText();
		Float width = 0;

		if ( mFont && !mFont->isMonospace() ) {
			auto& line = mDoc->line( docLine );
			auto found = mLinesWidthCache.find( docLine );
			if ( found != mLinesWidthCache.end() && line.getHash() == found->second.first )
				return found->second.second;
		}

		for ( size_t i = 0; i < vline.visualLines.size(); i++ ) {
			auto pos = vline.visualLines[i].column();
			auto len =
				i + 1 < vline.visualLines.size() ? vline.visualLines[i + 1].column() : line.size();
			auto vlineStr = line.view().substr( pos, len - pos );
			auto curWidth = getTextWidth( vlineStr );
			width = eemax( width, curWidth );
		}

		if ( mFont && !mFont->isMonospace() ) {
			width += getGlyphWidth();
			mLinesWidthCache[docLine] = { line.getHash(), width };
		}

		return width;
	}

	if ( mFont && !mFont->isMonospace() ) {
		auto& line = mDoc->line( docLine );
		auto found = mLinesWidthCache.find( docLine );
		if ( found != mLinesWidthCache.end() && line.getHash() == found->second.first )
			return found->second.second;
		Float width = getTextWidth( line.getText() ) + getGlyphWidth();
		mLinesWidthCache[docLine] = { line.getHash(), width };
		return width;
	}
	return getTextWidth( mDoc->line( docLine ).getText() );
}

void UICodeEditor::updateScrollBar() {
	int notVisibleLineCount = (int)mDoc->linesCount() - (int)getViewPortLineCount().y;

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

void UICodeEditor::copyFilePath( bool copyPosition ) {
	auto clipboard = getUISceneNode()->getWindow()->getClipboard();
	if ( copyPosition )
		clipboard->setText( mDoc->getFilePath() + mDoc->getSelection().start().toPositionString() );
	else
		clipboard->setText( mDoc->getFilePath() );
}

void UICodeEditor::scrollToCursor( bool centered ) {
	scrollTo( mDoc->getSelection().start(), centered );
}

void UICodeEditor::updateEditor() {
	mDoc->setPageSize( getViewPortLineCount().y );
	if ( mDirtyScroll && mDoc->getActiveClient() == this )
		scrollTo( mDoc->getSelection().start() );

	updateScrollBar();
	mDirtyEditor = false;
	mDirtyScroll = false;
}

void UICodeEditor::onDocumentTextChanged( const DocumentContentChange& change ) {
	invalidateDraw();
	checkMatchingBrackets();
	sendCommonEvent( Event::OnTextChanged );
	mLineWrapping.updateBreaks( change.range.start().line(), change.range.start().line(), 0 );

	if ( !change.text.empty() ) {
		auto range = findLongestLineInRange( change.range );
		if ( range.second > mLongestLineWidth ) {
			mLongestLineIndex = range.first;
			mLongestLineWidth = range.second;
		}
	} else {
		invalidateLongestLineWidth();
	}
}

void UICodeEditor::onDocumentCursorChange( const Doc::TextPosition& ) {
	resetCursor();
	checkMatchingBrackets();
	invalidateEditor();
	updateIMELocation();
	invalidateDraw();
	onCursorPosChange();
}

void UICodeEditor::onDocumentInterestingCursorChange( const TextPosition& ) {
	sendCommonEvent( Event::OnCursorPosChangeInteresting );
	invalidateDraw();
}

void UICodeEditor::onDocumentSelectionChange( const Doc::TextRange& ) {
	resetCursor();
	invalidateDraw();
	sendCommonEvent( Event::OnSelectionChanged );
}

void UICodeEditor::onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount ) {
	updateScrollBar();

	if ( Math::countDigits( (Int64)lastCount ) != Math::countDigits( (Int64)newCount ) )
		invalidateLineWrapMaxWidth( false );
}

void UICodeEditor::onDocumentLineChanged( const Int64& lineNumber ) {
	mDoc->getHighlighter()->invalidate( lineNumber );

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

void UICodeEditor::onDocumentLineMove( const Int64& fromLine, const Int64& toLine,
									   const Int64& numLines ) {
	mLineWrapping.updateBreaks( fromLine, toLine, numLines );

	if ( !mFont || mFont->isMonospace() || mLinesWidthCache.empty() )
		return;

	Int64 linesCount = mDoc->linesCount();
	if ( numLines > 0 ) {
		for ( Int64 i = linesCount - 1; i >= fromLine; --i ) {
			auto lineIt = mLinesWidthCache.find( i - numLines );
			if ( lineIt != mLinesWidthCache.end() ) {
				const auto& line = lineIt->second;
				if ( line.first == mDoc->line( i ).getHash() ) {
					auto nl = mLinesWidthCache.extract( lineIt );
					nl.key() = i;
					mLinesWidthCache.insert( std::move( nl ) );
				}
			}
		}
	} else if ( numLines < 0 ) {
		for ( Int64 i = fromLine; i < linesCount; i++ ) {
			auto lineIt = mLinesWidthCache.find( i - numLines );
			if ( lineIt != mLinesWidthCache.end() &&
				 lineIt->second.first == mDoc->line( i ).getHash() ) {
				auto nl = mLinesWidthCache.extract( lineIt );
				nl.key() = i;
				mLinesWidthCache[i] = std::move( nl.mapped() );
			}
		}
	}
}

void UICodeEditor::onDocumentDirtyOnFileSystem( TextDocument* doc ) {
	DocEvent event( this, doc, Event::OnDocumentDirtyOnFileSysten );
	sendEvent( &event );
}

std::pair<Uint64, Uint64> UICodeEditor::getVisibleLineRange( bool visualIndexes ) const {
	Float lineHeight = getLineHeight();
	Int64 minLine = eemax( 0.f, eefloor( mScroll.y / lineHeight ) );
	Int64 maxLine = eemin( eemax( getTotalVisibleLines() - 1.f, 0.f ),
						   eefloor( ( mSize.getHeight() + mScroll.y ) / lineHeight ) + 1 );
	if ( !visualIndexes && mLineWrapping.isWrapEnabled() ) {
		return std::make_pair<Uint64, Uint64>(
			(Uint64)mLineWrapping.getDocumentLine( minLine ).line(),
			(Uint64)mLineWrapping.getDocumentLine( maxLine ).line() );
	}
	return std::make_pair<Uint64, Uint64>( (Uint64)minLine, (Uint64)maxLine );
}

TextRange UICodeEditor::getVisibleRange() const {
	auto visibleLineRange = getVisibleLineRange();
	return mDoc->sanitizeRange( TextRange(
		TextPosition(
			visibleLineRange.first,
			mDoc->endOfLine( { static_cast<Int64>( visibleLineRange.first ), 0 } ).column() ),
		TextPosition(
			visibleLineRange.second,
			mDoc->endOfLine( { static_cast<Int64>( visibleLineRange.second ), 0 } ).column() ) ) );
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

void UICodeEditor::scrollTo( TextPosition position, bool centered, bool forceExactPosition,
							 bool scrollX ) {
	scrollTo( { position, position }, centered, forceExactPosition, scrollX );
}

void UICodeEditor::scrollToVisualIndex( Int64 visualIndex, bool centered,
										bool forceExactPosition ) {
	Rectf scrollArea = getVisibleScrollArea();
	Float lineHeight = getLineHeight();
	Int64 minDistance = mHScrollBar->isVisible() ? 3 : 2;
	scrollArea.Top += lineHeight;
	scrollArea.Bottom = eemax( scrollArea.Top, scrollArea.Bottom - minDistance * lineHeight );

	Vector2f offsetEnd{ 0.f, lineHeight * visualIndex };

	// Vertical Scroll
	Float halfScreenSize = eefloor( getViewportDimensions().getHeight() * 0.5f );
	if ( centered || forceExactPosition ) {
		setScrollY(
			eeclamp( offsetEnd.y - ( centered ? halfScreenSize : 0 ), 0.f, getMaxScroll().y ) );
	} else if ( offsetEnd.y < scrollArea.Top ) {
		setScrollY( eeclamp( offsetEnd.y - lineHeight, 0.f, getMaxScroll().y ) );
	} else if ( offsetEnd.y > scrollArea.Bottom - lineHeight ) {
		setScrollY(
			eeclamp( offsetEnd.y - scrollArea.getHeight() - lineHeight, 0.f, getMaxScroll().y ) );
	}
}

void UICodeEditor::scrollTo( TextRange position, bool centered, bool forceExactPosition,
							 bool scrollX ) {
	position.normalize();
	Rectf scrollArea = getVisibleScrollArea();
	Vector2f offsetStart = getTextPositionOffset( position.start() );
	Vector2f offsetEnd = getTextPositionOffset( position.end() );

	Float lineHeight = getLineHeight();
	Int64 minDistance = mHScrollBar->isVisible() ? 3 : 2;
	scrollArea.Top += lineHeight;
	scrollArea.Bottom = eemax( scrollArea.Top, scrollArea.Bottom - minDistance * lineHeight );

	if ( forceExactPosition || !scrollArea.contains( offsetEnd ) ) {
		// Vertical Scroll
		Float halfScreenSize = eefloor( getViewportDimensions().getHeight() * 0.5f );
		if ( forceExactPosition || centered ) {
			setScrollY(
				eeclamp( offsetEnd.y - ( centered ? halfScreenSize : 0 ), 0.f, getMaxScroll().y ) );
		} else if ( offsetEnd.y < scrollArea.Top ) {
			setScrollY( eeclamp( offsetEnd.y - lineHeight, 0.f, getMaxScroll().y ) );
		} else if ( offsetEnd.y > scrollArea.Bottom - lineHeight ) {
			setScrollY( eeclamp( offsetEnd.y - scrollArea.getHeight() - lineHeight, 0.f,
								 getMaxScroll().y ) );
		}
	}

	// Horizontal Scroll
	if ( !scrollX )
		return;
	Float minVisibility = getGlyphWidth();
	Float viewPortWidth = getViewportWidth();
	if ( viewPortWidth == 0 )
		return;
	if ( offsetEnd.x + minVisibility > mScroll.x + viewPortWidth ) {
		setScrollX( eefloor( eemax( 0.f, offsetEnd.x + minVisibility - viewPortWidth ) ) );
	} else if ( offsetEnd.x < mScroll.x ) {
		if ( offsetEnd.x - minVisibility < viewPortWidth )
			setScrollX( 0.f );
		else {
			Float offsetXStart = offsetStart.x;
			setScrollX( eefloor( eemax( 0.f, offsetXStart - minVisibility ) ) );
		}
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
	if ( showMinimap != mMinimapEnabled ) {
		mMinimapEnabled = showMinimap;
		invalidateLineWrapMaxWidth( true );
	}
}

void UICodeEditor::setScrollX( const Float& val, bool emmitEvent ) {
	Float oldVal = mScroll.x;
	mScroll.x = eefloor( eeclamp<Float>( val, 0.f, getMaxScroll().x ) );
	if ( oldVal != mScroll.x ) {
		invalidateDraw();
		updateIMELocation();
		if ( emmitEvent )
			sendCommonEvent( Event::OnScrollChange );
		if ( mHorizontalScrollBarEnabled && emmitEvent )
			mHScrollBar->setValue( mScroll.x / getMaxScroll().x, false );
	}
}

void UICodeEditor::setScrollY( const Float& val, bool emmitEvent ) {
	Float oldVal = mScroll.y;
	mScroll.y = eefloor( eeclamp<Float>( val, 0, getMaxScroll().y ) );
	if ( oldVal != mScroll.y ) {
		invalidateDraw();
		updateIMELocation();
		if ( emmitEvent )
			sendCommonEvent( Event::OnScrollChange );
		if ( mVerticalScrollBarEnabled && emmitEvent )
			mVScrollBar->setValue( mScroll.y / getMaxScroll().y, false );
	}
}

Vector2f UICodeEditor::getTextPositionOffset( const TextPosition& position,
											  std::optional<Float> lineHeight,
											  bool allowVisualLineEnd ) const {
	Float lh = lineHeight ? *lineHeight : getLineHeight();
	if ( mLineWrapping.isWrappedLine( position.line() ) ) {
		auto info = mLineWrapping.getVisualLineInfo( position, allowVisualLineEnd );
		if ( mFont && !mFont->isMonospace() ) {
			const auto& line = mDoc->line( position.line() ).getText();
			auto partialLine =
				line.view().substr( info.range.start().column(), info.range.end().column() );
			return { Text::findCharacterPos( position.column() - info.range.start().column(), mFont,
											 getCharacterSize(), partialLine,
											 mFontStyleConfig.Style, mTabWidth,
											 mFontStyleConfig.OutlineThickness, false )
						 .x,
					 info.visualIndex * lh };
		}
		const String& line = mDoc->line( position.line() ).getText();
		Float glyphWidth = getGlyphWidth();
		Float x = 0;
		Int64 maxCol = eemin( position.column(), info.range.end().column() );
		for ( auto i = info.range.start().column(); i < maxCol; i++ ) {
			if ( line[i] == '\t' ) {
				x += glyphWidth * mTabWidth;
			} else if ( line[i] != '\n' && line[i] != '\r' ) {
				x += glyphWidth;
			}
		}
		auto firstWrappedIndex = mLineWrapping.toWrappedIndex( position.line() );
		if ( info.visualIndex != firstWrappedIndex ) {
			x += mLineWrapping.getLineOffset( position.line() );
		}
		return { x, info.visualIndex * lh };
	}

	if ( mFont && !mFont->isMonospace() ) {
		return { Text::findCharacterPos(
					 ( position.column() == (Int64)mDoc->line( position.line() ).getText().size() )
						 ? position.column() - 1
						 : position.column(),
					 mFont, getCharacterSize(), mDoc->line( position.line() ).getText(),
					 mFontStyleConfig.Style, mTabWidth, mFontStyleConfig.OutlineThickness, false )
					 .x,
				 lh * mLineWrapping.toWrappedIndex( position.line() ) };
	}

	const String& line = mDoc->line( position.line() ).getText();
	Float glyphWidth = getGlyphWidth();
	Float x = 0;
	Int64 maxCol = eemin( (Int64)line.size(), position.column() );
	for ( auto i = 0; i < maxCol; i++ ) {
		if ( line[i] == '\t' ) {
			x += glyphWidth * mTabWidth;
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			x += glyphWidth;
		}
	}
	return { x, lh * mLineWrapping.toWrappedIndex( position.line() ) };
}

template <typename StringType> size_t UICodeEditor::characterWidth( const StringType& str ) const {
	Int64 cc = str.size();
	Int64 count = 0;
	for ( Int64 i = 0; i < cc; i++ )
		count += str[i] == '\t' ? mTabWidth : 1;
	return count;
}

size_t UICodeEditor::characterWidth( const String& str ) const {
	return characterWidth<String>( str );
}

Float UICodeEditor::getTextWidth( const String& text ) const {
	return getTextWidth<String>( text );
}

size_t UICodeEditor::characterWidth( const String::View& str ) const {
	return characterWidth<String::View>( str );
}

Float UICodeEditor::getTextWidth( const String::View& text ) const {
	return getTextWidth<String::View>( text );
}

template <typename StringType> Float UICodeEditor::getTextWidth( const StringType& line ) const {
	if ( mFont && !mFont->isMonospace() ) {
		return Text::getTextWidth( mFont, getCharacterSize(), line, mFontStyleConfig.Style,
								   mTabWidth );
	}

	Float glyphWidth = getGlyphWidth();
	size_t len = line.length();
	Float x = 0;
	for ( size_t i = 0; i < len; i++ )
		x += ( line[i] == '\t' ) ? glyphWidth * mTabWidth : glyphWidth;
	return x;
}

Vector2f UICodeEditor::getTextPositionOffsetSanitized( TextPosition position,
													   std::optional<Float> lineHeight ) const {
	position.setLine( eeclamp<Int64>( position.line(), 0L, mDoc->linesCount() - 1 ) );
	// This is different from sanitizePosition, sinze allows the last character.
	position.setColumn(
		eeclamp<Int64>( position.column(), 0L,
						eemax<Int64>( 0, position.line() < static_cast<Int64>( mDoc->linesCount() )
											 ? mDoc->line( position.line() ).size()
											 : 0 ) ) );
	return getTextPositionOffset( position, lineHeight );
}

const bool& UICodeEditor::isLocked() const {
	return mLocked;
}

void UICodeEditor::setLocked( bool locked ) {
	if ( mLocked != locked ) {
		mLocked = locked;
		if ( !mLocked && hasFocus() ) {
			resetCursor();
			getUISceneNode()->getWindow()->startTextInput();
			mDoc->setActiveClient( this );
			updateIMELocation();
		}
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
		if ( mLineWrapType == LineWrapType::LineBreakingColumn ) {
			if ( mLineBreakingColumn )
				invalidateLineWrapMaxWidth( false );
			else
				mLineWrapType = LineWrapType::Viewport;
		}
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
			Font* font = FontManager::instance()->getByName( attribute.value() );
			if ( NULL != font && font->loaded() ) {
				setFont( font );
			}
			break;
		}
		case PropertyId::FontSize:
			setFontSize( lengthFromValue( attribute ) );
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
			return String::fromFloat( getFontSize(), "px" );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::TextStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getOutlineThickness() ), "px" );
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
	if ( &definition == &mDoc->getSyntaxDefinition() )
		return;
	std::string oldLang( mDoc->getSyntaxDefinition().getLanguageName() );
	mDoc->getHighlighter()->reset();
	mDoc->setSyntaxDefinition( definition );
	if ( mMinimapEnabled && getUISceneNode()->hasThreadPool() ) {
		mDoc->getHighlighter()->tokenizeAsync( getUISceneNode()->getThreadPool(), [this] {
			runOnMainThread( [this] { invalidateDraw(); } );
		} );
	}
	invalidateDraw();
	DocSyntaxDefEvent event( this, mDoc.get(), Event::OnDocumentSyntaxDefinitionChange, oldLang,
							 mDoc->getSyntaxDefinition().getLanguageName() );
	sendEvent( &event );
}

void UICodeEditor::resetSyntaxDefinition() {
	std::string oldLang( mDoc->getSyntaxDefinition().getLanguageName() );
	mDoc->resetSyntax();
	if ( oldLang != mDoc->getSyntaxDefinition().getLanguageName() ) {
		mDoc->getHighlighter()->reset();
		invalidateDraw();
		DocSyntaxDefEvent event( this, mDoc.get(), Event::OnDocumentSyntaxDefinitionChange, oldLang,
								 mDoc->getSyntaxDefinition().getLanguageName() );
		sendEvent( &event );
	}
}

const SyntaxDefinition& UICodeEditor::getSyntaxDefinition() const {
	return mDoc->getSyntaxDefinition();
}

void UICodeEditor::checkMatchingBrackets() {
	if ( mHighlightMatchingBracket ) {
		static const std::vector<String::StringBaseType> open{ '{', '(', '[' };
		static const std::vector<String::StringBaseType> close{ '}', ')', ']' };
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
			TextPosition closePosition = mDoc->getMatchingBracket(
				pos, openBracket, closeBracket, TextDocument::MatchDirection::Forward );
			mMatchingBrackets = { pos, closePosition };
		} else if ( isCloseIt != close.end() ) {
			size_t index = std::distance( close.begin(), isCloseIt );
			String::StringBaseType openBracket = open[index];
			String::StringBaseType closeBracket = close[index];
			TextPosition closePosition = mDoc->getMatchingBracket(
				pos, openBracket, closeBracket, TextDocument::MatchDirection::Backward );
			mMatchingBrackets = { pos, closePosition };
		}
	}
}

Int64 UICodeEditor::getColFromXOffset( Int64 visualLine, const Float& x ) const {
	if ( x < 0 || !mFont || mDoc->isLoading() )
		return 0;

	TextPosition pos = mDoc->sanitizePosition( mLineWrapping.getDocumentLine( visualLine ) );

	if ( mLineWrapping.isWrappedLine( pos.line() ) ) {
		auto visualRange = mLineWrapping.getVisualLineRange( visualLine );
		auto firstVisualIndex = mLineWrapping.toWrappedIndex( visualRange.start().line() );
		Float xOffset = firstVisualIndex != visualLine
							? mLineWrapping.getLineOffset( visualRange.start().line() )
							: 0;

		auto line = mDoc->line( visualRange.start().line() )
						.getText()
						.view()
						.substr( visualRange.start().column(), visualRange.length() );

		if ( mFont && !mFont->isMonospace() ) {
			return visualRange.start().column() +
				   Text::findCharacterFromPos( Vector2i( eemax( -xOffset + x, 0.f ), 0 ), true,
											   mFont, getCharacterSize(), line,
											   mFontStyleConfig.Style, mTabWidth );
		}

		Int64 len = line.length();
		Float glyphWidth = getGlyphWidth();
		Float tabWidth = glyphWidth * mTabWidth;
		Float hTab = tabWidth * 0.5f;
		for ( int i = 0; i < len; i++ ) {
			bool isTab = ( line[i] == '\t' );
			if ( xOffset >= x ) {
				auto col = xOffset - x > ( isTab ? hTab : glyphWidth * 0.5f )
							   ? eemax<Int64>( 0, i - 1 )
							   : i;
				return visualRange.start().column() + col;
			} else if ( isTab && ( xOffset + tabWidth > x ) ) {
				auto col = x - xOffset > hTab ? eemin<Int64>( i + 1, line.size() - 1 ) : i;
				return visualRange.start().column() + col;
			}
			xOffset += isTab ? tabWidth : glyphWidth;
		}
		return visualRange.start().column() + static_cast<Int64>( line.size() ) - 1;
	}

	if ( mFont && !mFont->isMonospace() ) {
		return Text::findCharacterFromPos( Vector2i( x, 0 ), true, mFont, getCharacterSize(),
										   mDoc->line( pos.line() ).getText(),
										   mFontStyleConfig.Style, mTabWidth );
	}

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
	return mFontStyleConfig.getFontCharacterSize();
}

Float UICodeEditor::getGlyphWidth() const {
	return mGlyphWidth;
}

void UICodeEditor::udpateGlyphWidth() {
	mGlyphWidth = mFont->getGlyph( '@', getCharacterSize(), false, false ).advance;
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
		xo.offset = getTextPositionOffsetSanitized( position ).x;

	if ( mLineWrapping.isWrapEnabled() ) {
		auto info = mLineWrapping.getVisualLineInfo( position );
		auto nextLine = mLineWrapping.getVisualLineRange( info.visualIndex + offset );
		xo.position.setLine( nextLine.end().line() );
		xo.position.setColumn( getColFromXOffset( info.visualIndex + offset, xo.offset ) );
		return xo.position;
	}

	xo.position.setLine( position.line() + offset );
	xo.position.setColumn( getColFromXOffset( position.line() + offset, xo.offset ) );
	return xo.position;
}

void UICodeEditor::moveToPreviousLine() {
	jumpLinesUp( -1 );
}

void UICodeEditor::moveToNextLine() {
	jumpLinesDown( 1 );
}

void UICodeEditor::moveToStartOfLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		auto selection = mDoc->getSelectionIndex( i );
		auto info = mLineWrapping.getVisualLineInfo( mDoc->getSelectionIndex( i ).start() );
		mDoc->setSelection( i, info.range.start() != selection.start()
								   ? info.range.start()
								   : mDoc->startOfLine( selection.start() ) );
	}
	mDoc->mergeSelection();
}

void UICodeEditor::moveToEndOfLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		auto selection = mDoc->getSelectionIndex( i );
		auto info = mLineWrapping.getVisualLineInfo( selection.start() );
		mDoc->setSelection( i, info.range.end() != selection.end()
								   ? info.range.end()
								   : mDoc->endOfLine( selection.end() ) );
	}
	mDoc->mergeSelection();
}

void UICodeEditor::moveToStartOfContent() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition start = mDoc->getSelectionIndex( i ).start();
		auto info = mLineWrapping.getVisualLineInfo( start );
		if ( info.range.start().column() != 0 ) {
			mDoc->setSelection( i, info.range.start() != start ? info.range.start()
															   : mDoc->startOfContent( start ) );
		} else {
			TextPosition indented = mDoc->startOfContent( mDoc->getSelectionIndex( i ).start() );
			mDoc->setSelection( i, indented.column() == start.column()
									   ? TextPosition( start.line(), 0 )
									   : indented );
		}
	}
	mDoc->mergeSelection();
}

void UICodeEditor::selectToStartOfLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		auto selection = mDoc->getSelectionIndex( i );
		auto info = mLineWrapping.getVisualLineInfo( mDoc->getSelectionIndex( i ).start() );
		mDoc->selectTo( i, info.range.start() != selection.start()
							   ? info.range.start()
							   : mDoc->startOfLine( selection.start() ) );
	}
	mDoc->mergeSelection();
}

void UICodeEditor::selectToEndOfLine() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		auto selection = mDoc->getSelectionIndex( i );
		auto info = mLineWrapping.getVisualLineInfo( selection.start() );
		mDoc->selectTo( i, info.range.end() != selection.start()
							   ? info.range.end()
							   : mDoc->endOfLine( selection.start() ) );
	}
	mDoc->mergeSelection();
}

void UICodeEditor::selectToStartOfContent() {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition start = mDoc->getSelectionIndex( i ).start();
		auto info = mLineWrapping.getVisualLineInfo( start );
		if ( info.range.start().column() != 0 ) {
			mDoc->selectTo( i, info.range.start() != start ? info.range.start()
														   : mDoc->startOfContent( start ) );
		} else {
			TextPosition indented = mDoc->startOfContent( mDoc->getSelectionIndex( i ).start() );
			mDoc->selectTo( i, indented.column() == start.column() ? TextPosition( start.line(), 0 )
																   : indented );
		}
	}
	mDoc->mergeSelection();
}

void UICodeEditor::jumpLinesUp( int offset ) {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition position = mDoc->getSelections()[i].start();
		if ( mLineWrapping.toWrappedIndex( position.line() ) == 0 ) {
			mDoc->setSelection( i, mDoc->startOfDoc(), mDoc->startOfDoc() );
		} else {
			mDoc->moveTo( i, moveToLineOffset( position, offset, i ) );
		}
	}
	mDoc->mergeSelection();
}

void UICodeEditor::jumpLinesDown( int offset ) {
	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		TextPosition position = mDoc->getSelections()[i].start();
		if ( position.line() >= (Int64)mDoc->linesCount() - offset ) {
			mDoc->setSelection( i, mDoc->endOfDoc(), mDoc->endOfDoc() );
		} else {
			mDoc->moveTo( i, moveToLineOffset( position, offset, i ) );
		}
	}
	mDoc->mergeSelection();
}

void UICodeEditor::jumpLinesUp() {
	jumpLinesDown( -mJumpLinesLength );
}

void UICodeEditor::jumpLinesDown() {
	jumpLinesDown( mJumpLinesLength );
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

size_t UICodeEditor::getJumpLinesLength() const {
	return mJumpLinesLength;
}

void UICodeEditor::setJumpLinesLength( size_t jumpLinesLength ) {
	mJumpLinesLength = jumpLinesLength;
}

std::string UICodeEditor::getFileLockIconName() const {
	return mFileLockIconName;
}

void UICodeEditor::setFileLockIconName( const std::string& fileLockIconName ) {
	if ( mFileLockIconName != fileLockIconName ) {
		mFileLockIconName = fileLockIconName;
		mFileLockIcon = nullptr;
	}
}

bool UICodeEditor::getDisplayLockedIcon() const {
	return mDisplayLockedIcon;
}

void UICodeEditor::setDisplayLockedIcon( bool displayLockedIcon ) {
	mDisplayLockedIcon = displayLockedIcon;
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
	mDoc->pasteText( String::fromUtf8( getUISceneNode()->getWindow()->getClipboard()->getText() ) );
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
	if ( mHighlightWord.isEmpty() )
		return;

	if ( getUISceneNode()->hasThreadPool() ) {
		Uint64 tag = reinterpret_cast<Uint64>( this );
		removeActionsByTag( tag );
		runOnMainThread(
			[this, tag]() {
				getUISceneNode()->getThreadPool()->removeWithTag( tag );
				getUISceneNode()->getThreadPool()->run(
					[this]() {
						if ( mDoc->isRunningTransaction() )
							return;
						Clock docSearch;
						mHighlightWordProcessing++;
						mDoc->stopActiveFindAll();

						auto wordCache = mDoc->findAll(
							mHighlightWord.escapeSequences ? String::unescape( mHighlightWord.text )
														   : mHighlightWord.text,
							mHighlightWord.caseSensitive, mHighlightWord.wholeWord,
							mHighlightWord.type, mHighlightWord.range );

						{
							Lock l( mHighlightWordCacheMutex );
							mHighlightWordCache = wordCache.ranges();
						}

						Log::info( "Document search triggered in document: \"%s\", searched for "
								   "\"%s\" and took %.2f ms",
								   mDoc->getFilename().c_str(),
								   mHighlightWord.text.toUtf8().c_str(),
								   docSearch.getElapsedTime().asMilliseconds() );
					},
					[this]( const auto& ) { mHighlightWordProcessing--; }, tag );
			},
			Milliseconds( 16 ), tag );
	} else {
		if ( mDoc->isRunningTransaction() )
			return;
		mHighlightWordCache =
			mDoc->findAll( mHighlightWord.escapeSequences ? String::unescape( mHighlightWord.text )
														  : mHighlightWord.text,
						   mHighlightWord.caseSensitive, mHighlightWord.wholeWord,
						   mHighlightWord.type, mHighlightWord.range, 100 )
				.ranges();
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
			auto offset = getTextPositionOffset( pos, lineHeight );
			primitive.drawRectangle(
				Rectf( Vector2f( startScroll.x + offset.x, startScroll.y + offset.y ),
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
	Rectf area = getScreenBounds();
	Int64 lastSkipLine = -1;

	for ( const auto& range : ranges ) {
		if ( !( range.start().line() >= lineRange.first &&
				range.end().line() <= lineRange.second ) )
			continue;

		if ( ignoreSelectionMatch && selection.inSameLine() &&
			 selection.start().line() == range.start().line() &&
			 selection.start().column() == range.start().column() ) {
			continue;
		}

		if ( ranges.isSorted() && range.start().line() == lastSkipLine )
			continue;

		Int64 startCol = range.start().column();
		Int64 endCol = range.end().column();

		auto rects = getTextRangeRectangles(
			{ { range.start().line(), startCol }, { range.start().line(), endCol } }, startScroll,
			{}, lineHeight );

		for ( const auto& rect : rects ) {
			if ( area.intersect( rect ) ) {
				primitives.drawRectangle( rect );
			} else if ( rect.Left > area.Right ) {
				lastSkipLine = range.start().line();
			}
		}
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
		if ( line.size() > EE_1KB )
			continue;

		do {
			pos = line.find( text, pos );
			const auto InvalidPos = String::InvalidPos;
			if ( pos != InvalidPos ) {
				if ( ignoreSelectionMatch ) {
					TextRange selection = mDoc->getSelection( true );
					if ( selection.inSameLine() && selection.start().line() == ln &&
						 selection.start().column() == (Int64)pos ) {
						pos = selection.end().column();
						continue;
					}
				}

				Int64 startCol = pos;
				Int64 endCol = pos + text.size();
				auto rects = getTextRangeRectangles( { { ln, startCol }, { ln, endCol } },
													 startScroll, {}, lineHeight );
				for ( const auto& rect : rects )
					primitives.drawRectangle( rect );

				pos = endCol;
			} else {
				break;
			}
		} while ( true );
	}
	primitives.setForceDraw( true );
}

void UICodeEditor::drawLineText( const Int64& line, Vector2f position, const Float& fontSize,
								 const Float& lineHeight,
								 const std::pair<Uint64, Uint64>& visualLineRange ) {
	Vector2f originalPosition( position );
	const auto& tokens = mDoc->getHighlighter()->getLine( line );
	const String& strLine = mDoc->line( line ).getText();
	Primitives primitives;
	Int64 curChar = 0;
	Int64 maxWidth = eeceil( mSize.getWidth() / getGlyphWidth() + 1 );
	bool isMonospace = mFont->isMonospace();
	bool isFallbackFont = false;
	bool isEmojiFallbackFont = false;
	bool ended = false;
	Float lineOffset = getLineOffset();
	size_t pos = 0;
	if ( mDoc->mightBeBinary() && mFont->getType() == FontType::TTF ) {
		FontTrueType* ttf = static_cast<FontTrueType*>( mFont );
		isFallbackFont = ttf->isFallbackFontEnabled();
		isEmojiFallbackFont = ttf->isEmojiFallbackEnabled();
		ttf->setEnableFallbackFont( false );
		ttf->setEnableEmojiFallback( false );
	}

	String::View buff;
	Sizef size;
	FontStyleConfig fontStyle( mFontStyleConfig );
	fontStyle.CharacterSize = fontSize;

	const auto drawHandDown = [this, &fontStyle, &lineHeight]() {
		auto rects = getTextRangeRectangles( mLinkPosition, getScreenScroll(), {}, lineHeight );
		auto screenBounds = getScreenBounds();
		if ( !std::any_of( rects.begin(), rects.end(), [&screenBounds]( const Rectf& rect ) {
				 return screenBounds.intersect( rect );
			 } ) ) {
			return;
		}

		if ( !mUseDefaultStyle ) {
			auto tokenType = mDoc->getHighlighter()->getTokenTypeAt( mLinkPosition.start() );
			const SyntaxColorScheme::Style& style = mColorScheme.getSyntaxStyle( tokenType );
			SyntaxColorScheme::Style linkStyle = style;

			fontStyle.Style = style.style;
			fontStyle.FontColor = Color( style.color ).blendAlpha( mAlpha );
			fontStyle.OutlineThickness = style.outlineThickness;
			if ( fontStyle.OutlineThickness )
				fontStyle.OutlineColor = style.outlineColor;

			if ( mColorScheme.hasSyntaxStyle( "link_hover"_sst ) ) {
				linkStyle = mColorScheme.getSyntaxStyle( "link_hover"_sst );
				if ( linkStyle.color != Color::Transparent ) {
					fontStyle.FontColor = Color( linkStyle.color ).blendAlpha( mAlpha );
				}
				fontStyle.Style = linkStyle.style;
			}
		}

		for ( auto& rect : rects ) {
			if ( screenBounds.intersect( rect ) ) {
				Text::drawUnderline( rect.getPosition(), rect.getWidth(), fontStyle.Font,
									 fontStyle.CharacterSize, fontStyle.FontColor, fontStyle.Style,
									 fontStyle.OutlineThickness, fontStyle.OutlineColor,
									 fontStyle.ShadowColor, fontStyle.ShadowOffset );
			}
		}
	};

	if ( mLineWrapping.isWrappedLine( line ) ) {
		auto vline = mLineWrapping.getVisualLine( line );
		size_t curvline = 1;
		size_t lineLength = strLine.length();
		size_t nextLineCol = vline.visualLines[curvline].column();
		size_t tokenPos;
		size_t curVisualIndex;
		for ( const auto& token : tokens ) {
			tokenPos = 0;
			curVisualIndex = vline.visualIndex + curvline;
			while ( tokenPos < token.len && nextLineCol != pos && !ended ) {
				Int64 maxLength = nextLineCol - pos;
				auto textSize = std::min( (Int64)( (Int64)token.len - tokenPos ), maxLength );
				String::View text = strLine.view().substr( pos, textSize );

				if ( curVisualIndex >= visualLineRange.first &&
					 curVisualIndex <= visualLineRange.second + 1 ) {
					if ( !mUseDefaultStyle ) {
						const SyntaxColorScheme::Style& style =
							mColorScheme.getSyntaxStyle( token.type );
						fontStyle.Style = style.style;
						fontStyle.FontColor = Color( style.color ).blendAlpha( mAlpha );
						fontStyle.OutlineThickness = style.outlineThickness;
						if ( fontStyle.OutlineThickness )
							fontStyle.OutlineColor = style.outlineColor;

						if ( style.background != Color::Transparent ) {
							primitives.setColor( Color( style.background ).blendAlpha( mAlpha ) );
							primitives.drawRectangle(
								Rectf( position, Sizef( getTextWidth( text ), lineHeight ) ) );
						}
					}

					position.x += Text::draw( text, { position.x, position.y + lineOffset },
											  fontStyle, mTabWidth )
									  .getWidth();
				}

				pos += text.size();
				tokenPos += text.size();

				if ( tokenPos < lineLength && tokenPos == token.len && nextLineCol != pos )
					break;

				position.y += lineHeight;
				position.x = originalPosition.x + vline.paddingStart;
				curvline++;
				curVisualIndex++;

				if ( curvline < vline.visualLines.size() ) {
					nextLineCol = vline.visualLines[curvline].column();
				} else {
					nextLineCol = strLine.size();
				}

				if ( curVisualIndex > visualLineRange.second + 1 ) {
					ended = true;
					break;
				}
			}
		}
	} else {
		for ( const auto& token : tokens ) {
			String::View text = strLine.view();
			if ( pos < strLine.size() && !( pos == 0 && text.size() == token.len ) ) {
				buff = strLine.view().substr( pos, token.len );
				text = buff;
			}
			pos += token.len;

			Float textWidth = isMonospace ? getTextWidth( text ) : 0;
			if ( !isMonospace || ( position.x + textWidth >= mScreenPos.x &&
								   position.x <= mScreenPos.x + mSize.getWidth() ) ) {
				Int64 curCharsWidth = text.size();
				Int64 curPositionChar = eefloor( mScroll.x / getGlyphWidth() );
				Float curMaxPositionChar = curPositionChar + maxWidth;
				if ( !mUseDefaultStyle ) {
					const SyntaxColorScheme::Style& style =
						mColorScheme.getSyntaxStyle( token.type );
					fontStyle.Style = style.style;
					fontStyle.FontColor = Color( style.color ).blendAlpha( mAlpha );
					fontStyle.OutlineThickness = style.outlineThickness;
					if ( fontStyle.OutlineThickness )
						fontStyle.OutlineColor = style.outlineColor;

					if ( style.background != Color::Transparent ) {
						primitives.setColor( Color( style.background ).blendAlpha( mAlpha ) );
						primitives.drawRectangle(
							Rectf( position, Sizef( textWidth, lineHeight ) ) );
					}
				}

				if ( isMonospace &&
					 curPositionChar + curChar + curCharsWidth > curMaxPositionChar ) {
					if ( curChar < curPositionChar ) {
						Int64 charsToVisible = curPositionChar - curChar;
						Int64 start = eemax( (Int64)0, curPositionChar - curChar );
						Int64 minimumCharsToCoverScreen = maxWidth + charsToVisible - start;
						Int64 totalChars = curCharsWidth - start;
						Int64 end = eemin( totalChars, minimumCharsToCoverScreen );
						if ( curCharsWidth >= charsToVisible ) {
							size = Text::draw(
								text.substr( start, end ),
								{ position.x + start * getGlyphWidth(), position.y + lineOffset },
								fontStyle, mTabWidth );
							if ( minimumCharsToCoverScreen == end )
								break;
						}
					} else {
						size = Text::draw( text.substr( 0, eemin( curCharsWidth, maxWidth ) ),
										   { position.x, position.y + lineOffset }, fontStyle,
										   mTabWidth );
					}
				} else {
					size = Text::draw( text, { position.x, position.y + lineOffset }, fontStyle,
									   mTabWidth );
				}

				if ( !isMonospace )
					textWidth = size.getWidth();
			} else if ( position.x > mScreenPos.x + mSize.getWidth() ) {
				break;
			}

			position.x += textWidth;
			curChar += characterWidth( text );
		}
	}

	if ( mHandShown && mLinkPosition.isValid() && mLinkPosition.inSameLine() &&
		 mLinkPosition.start().line() == line ) {
		drawHandDown();
	}

	if ( mDoc->mightBeBinary() && mFont->getType() == FontType::TTF ) {
		FontTrueType* ttf = static_cast<FontTrueType*>( mFont );
		ttf->setEnableFallbackFont( isFallbackFont );
		ttf->setEnableEmojiFallback( isEmojiFallbackFont );
	}
}

std::vector<Rectf>
UICodeEditor::getTextRangeRectangles( const TextRange& range, const Vector2f& startScroll,
									  std::optional<const std::pair<int, int>> lineRange,
									  std::optional<Float> lineHeight ) {
	std::vector<Rectf> rects;
	Float lh = lineHeight ? *lineHeight : getLineHeight();
	int startLine =
		eemax<int>( lineRange ? lineRange->first : range.start().line(), range.start().line() );
	int endLine =
		eemin<int>( lineRange ? lineRange->second : range.end().line(), range.end().line() );
	for ( auto ln = startLine; ln <= endLine; ln++ ) {
		const String& line = mDoc->line( ln ).getText();
		Rectf selRect;
		if ( mLineWrapping.isWrappedLine( ln ) ) {
			auto fromInfo = mLineWrapping.getVisualLineInfo(
				ln == range.start().line() ? range.start() : mDoc->startOfLine( { ln, 0 } ) );
			auto toInfo = mLineWrapping.getVisualLineInfo(
				ln == range.end().line() ? range.end() : mDoc->endOfLine( { ln, 0 } ) );
			for ( Int64 visibleIdx = fromInfo.visualIndex; visibleIdx <= toInfo.visualIndex;
				  visibleIdx++ ) {
				auto info = mLineWrapping.getDocumentLine( visibleIdx );
				Vector2f startOffset;
				Vector2f endOffset;
				if ( ln == range.start().line() && fromInfo.range.start().line() == ln &&
					 fromInfo.visualIndex == visibleIdx ) {
					startOffset = getTextPositionOffset( range.start(), lh );
				} else {
					startOffset = getTextPositionOffset( info, lh );
				}
				selRect.Top = startScroll.y + startOffset.y;
				selRect.Bottom = selRect.Top + lh;
				selRect.Left = startScroll.x + startOffset.x;
				if ( ln == range.end().line() && toInfo.range.end().line() == ln &&
					 toInfo.visualIndex == visibleIdx ) {
					endOffset = getTextPositionOffset( range.end(), lh );
				} else {
					auto nextInfo = mLineWrapping.getDocumentLine( visibleIdx + 1 );
					if ( nextInfo.line() == info.line() ) {
						endOffset =
							getTextPositionOffset( { info.line(), nextInfo.column() }, lh, true );
					} else {
						endOffset = getTextPositionOffset( mDoc->endOfLine( { ln, 0 } ), lh, true );
					}
				}
				selRect.Right = startScroll.x + endOffset.x;
				rects.push_back( selRect );
			}
		} else {
			if ( range.start().line() == ln ) {
				auto startOffset = getTextPositionOffset( { ln, range.start().column() }, lh );
				selRect.Top = startScroll.y + startOffset.y;
				selRect.Bottom = selRect.Top + lh;
				selRect.Left = startScroll.x + startOffset.x;
				if ( range.end().line() == ln ) {
					selRect.Right =
						startScroll.x + getTextPositionOffset( { ln, range.end().column() }, lh ).x;
				} else {
					selRect.Right =
						startScroll.x +
						getTextPositionOffset( { ln, static_cast<Int64>( line.length() ) }, lh ).x;
				}
			} else if ( range.end().line() == ln ) {
				auto startOffset = getTextPositionOffset( { ln, 0 }, lh );
				selRect.Top = startScroll.y + startOffset.y;
				selRect.Bottom = selRect.Top + lh;
				selRect.Left = startScroll.x + startOffset.x;
				selRect.Right =
					startScroll.x + getTextPositionOffset( { ln, range.end().column() }, lh ).x;
			} else {
				auto startOffset = getTextPositionOffset( { ln, 0 }, lh );
				selRect.Top = startScroll.y + startOffset.y;
				selRect.Bottom = selRect.Top + lh;
				selRect.Left = startScroll.x + startOffset.x;
				selRect.Right =
					startScroll.x +
					getTextPositionOffset( { ln, static_cast<Int64>( line.length() ) }, lh ).x;
			}
			rects.push_back( selRect );
		}
	}
	return rects;
}

void UICodeEditor::drawTextRange( const TextRange& range, const std::pair<int, int>& lineRange,
								  const Vector2f& startScroll, const Float& lineHeight,
								  const Color& backgroundColor ) {
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( backgroundColor ).blendAlpha( mAlpha ) );
	auto rects = getTextRangeRectangles( range, startScroll, lineRange, lineHeight );
	for ( const auto& rect : rects )
		primitives.drawRectangle( rect );
	if ( !rects.empty() )
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
		Text::draw( pos,
					Vector2f( screenStart.x + mLineNumberPaddingLeft,
							  startScroll.y +
								  lineHeight * (double)mLineWrapping.toWrappedIndex( i ) +
								  lineOffset ),
					mFontStyleConfig.Font, fontSize,
					( i >= selection.start().line() && i <= selection.end().line() )
						? mLineNumberActiveFontColor
						: mLineNumberFontColor,
					mFontStyleConfig.Style, mFontStyleConfig.OutlineThickness,
					mFontStyleConfig.OutlineColor, mFontStyleConfig.ShadowColor,
					mFontStyleConfig.ShadowOffset );
	}
}

void UICodeEditor::drawColorPreview( const Vector2f& startScroll, const Float& lineHeight ) {
	Primitives primitives;
	primitives.setColor( mPreviewColor );
	auto rects = getTextRangeRectangles( mPreviewColorRange, startScroll, {}, lineHeight );
	for ( const auto& rect : rects ) {
		primitives.drawRectangle( Rectf( Vector2f( rect.Left + mScroll.x, rect.Top + lineHeight ),
										 Sizef( rect.getWidth(), lineHeight * 2 ) ) );
	}
}

void UICodeEditor::drawWhitespaces( const std::pair<int, int>& lineRange,
									const Vector2f& startScroll, const Float& lineHeight ) {
	Float tabWidth = getTextWidth( String( "\t" ) );
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
		const auto& text = mDoc->line( index ).getText();
		if ( mLineWrapping.isWrappedLine( index ) ) {
			for ( size_t i = 0; i < text.size(); i++ ) {
				if ( ' ' == text[i] ) {
					auto offset =
						startScroll + getTextPositionOffset( { index, static_cast<Int64>( i ) } );
					cpoint->draw( offset );
				} else if ( '\t' == text[i] ) {
					auto offset =
						startScroll + getTextPositionOffset( { index, static_cast<Int64>( i ) } );
					offset.x += tabCenter;
					adv->draw( offset );
				}
			}
		} else {
			Vector2f position(
				{ startScroll.x,
				  startScroll.y + lineHeight * mLineWrapping.toWrappedIndex( index ) } );
			for ( size_t i = 0; i < text.size(); i++ ) {
				if ( position.x + mScroll.x + ( text[i] == '\t' ? tabWidth : glyphW ) >=
						 mScreenPos.x &&
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
	int spaceW = getTextWidth( String( " " ) );
	p.setColor( Color( mWhitespaceColor ).blendAlpha( mAlpha ) );
	int indentSize = mDoc->getIndentType() == TextDocument::IndentType::IndentTabs
						 ? getTabWidth()
						 : mDoc->getIndentWidth();
	for ( int index = lineRange.first; index <= lineRange.second; index++ ) {
		Vector2f position(
			{ startScroll.x, startScroll.y + lineHeight * mLineWrapping.toWrappedIndex( index ) } );
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
							 startScroll.y + lineHeight * mLineWrapping.toWrappedIndex( index ) } );
		nl->draw( Vector2f( position.x, position.y ) );
	}
}

void UICodeEditor::registerCommands() {
	mDoc->setCommand( "move-to-previous-line", [this] { moveToPreviousLine(); } );
	mDoc->setCommand( "move-to-next-line", [this] { moveToNextLine(); } );
	mDoc->setCommand( "move-to-start-of-line", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->moveToStartOfLine();
	} );
	mDoc->setCommand( "move-to-end-of-line", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->moveToEndOfLine();
	} );
	mDoc->setCommand( "move-to-start-of-content", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->moveToStartOfContent();
	} );
	mDoc->setCommand( "select-to-previous-line", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->selectToPreviousLine();
	} );
	mDoc->setCommand( "select-to-next-line", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->selectToNextLine();
	} );
	mDoc->setCommand( "select-to-start-of-line", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->selectToStartOfLine();
	} );
	mDoc->setCommand( "select-to-end-of-line", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->selectToEndOfLine();
	} );
	mDoc->setCommand( "select-to-start-of-content", []( Client* client ) {
		static_cast<UICodeEditor*>( client )->selectToStartOfContent();
	} );
	mDoc->setCommand( "move-scroll-up", [this] { moveScrollUp(); } );
	mDoc->setCommand( "move-scroll-down", [this] { moveScrollDown(); } );
	mDoc->setCommand( "jump-lines-up", [this] { jumpLinesUp(); } );
	mDoc->setCommand( "jump-lines-down", [this] { jumpLinesDown(); } );
	mDoc->setCommand( "indent", [this] { indent(); } );
	mDoc->setCommand( "unindent", [this] { unindent(); } );
	mDoc->setCommand( "copy", [this] { copy(); } );
	mDoc->setCommand( "cut", [this] { cut(); } );
	mDoc->setCommand( "paste", [this] { paste(); } );
	mDoc->setCommand( "font-size-grow", [this] { fontSizeGrow(); } );
	mDoc->setCommand( "font-size-shrink", [this] { fontSizeShrink(); } );
	mDoc->setCommand( "font-size-reset", [this] { fontSizeReset(); } );
	mDoc->setCommand( "lock", [this] { setLocked( true ); } );
	mDoc->setCommand( "unlock", [this] { setLocked( false ); } );
	mDoc->setCommand( "lock-toggle", [this] { setLocked( !isLocked() ); } );
	mDoc->setCommand( "open-containing-folder", [this] { openContainingFolder(); } );
	mDoc->setCommand( "copy-containing-folder-path", [this] { copyContainingFolderPath(); } );
	mDoc->setCommand( "copy-file-path", [this] { copyFilePath(); } );
	mDoc->setCommand( "copy-file-path-and-position", [this] { copyFilePath( true ); } );
	mDoc->setCommand( "find-replace", [this] { showFindReplace(); } );
	mDoc->setCommand( "open-context-menu", [this] { createContextMenu(); } );
	mUnlockedCmd.insert( { "copy", "select-all", "open-containing-folder",
						   "copy-containing-folder-path", "copy-file-path",
						   "copy-file-path-and-position", "open-context-menu", "find-replace" } );
}

void UICodeEditor::showFindReplace() {
	UISceneNode* curSceneNode = SceneManager::instance()->getUISceneNode();
	if ( mUISceneNode != curSceneNode )
		SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );

	if ( !mFindReplaceEnabled )
		return;
	if ( nullptr == mFindReplace )
		mFindReplace = UIDocFindReplace::New( this, mDoc );
	mFindReplace->setReplaceDisabled( mLocked );
	mFindReplace->show();

	if ( mUISceneNode != curSceneNode )
		SceneManager::instance()->setCurrentUISceneNode( curSceneNode );
}

Tools::UIDocFindReplace* UICodeEditor::getFindReplace() {
	return mFindReplace;
}

void UICodeEditor::registerKeybindings() {
	mKeyBindings.addKeybinds( getDefaultKeybindings() );
}

void UICodeEditor::onCursorPosChange() {
	mLastActivity.restart();
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
	if ( !mInteractiveLinks || !getUISceneNode()->getWindow()->getInput()->isKeyModPressed() )
		return resetLinkOver();

	TextPosition pos( resolveScreenPosition( position.asFloat(), false ) );
	if ( pos.line() >= (Int64)mDoc->linesCount() )
		return resetLinkOver();

	const String& line = mDoc->line( pos.line() ).getText();
	if ( pos.column() >= (Int64)line.size() - 1 )
		return resetLinkOver();

	if ( mDoc->getChar( pos ) == '\n' )
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

static const SyntaxStyleType SYNTAX_NORMAL = SyntaxStyleTypes::Normal;

void UICodeEditor::drawMinimap( const Vector2f& start, const std::pair<Uint64, Uint64>& ) {
	Float charHeight = PixelDensity::getPixelDensity() * mMinimapConfig.scale;
	Float charSpacing =
		eemax( 1.f, eefloor( 0.8 * PixelDensity::getPixelDensity() * mMinimapConfig.scale ) );
	Float lineSpacing = getMinimapLineSpacing();
	Rectf rect( getMinimapRect( start ) );
	auto visibleLineRange = getVisibleLineRange( true );
	int visibleLinesCount = ( visibleLineRange.second - visibleLineRange.first );
	int visibleLinesStart = visibleLineRange.first;
	Float scrollerHeight = visibleLinesCount * lineSpacing;
	int lineCount = getTotalVisibleLines();
	Float visibleY = rect.Top + visibleLinesStart * lineSpacing;
	int maxMinmapLines = eefloor( rect.getHeight() / lineSpacing );
	int minimapStartLine = 0;

	if ( isMinimapFileTooLarge() && visibleLinesCount ) {
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

	// Disable multi-sample to avoid rectangle-smoothing
	bool disableMultisample = !mMinimapConfig.allowSmoothing && GLi->isMultisample();
	ScopedOp op(
		[disableMultisample] {
			if ( disableMultisample )
				GLi->multisample( false );
		},
		[disableMultisample] {
			if ( disableMultisample )
				GLi->multisample( true );
		} );

	GlobalBatchRenderer* BR = GlobalBatchRenderer::instance();
	BR->setTexture( nullptr );
	BR->setBlendMode( BlendMode::Alpha() );
	BR->quadsBegin();

	if ( mMinimapConfig.drawBackground ) {
		BR->quadsSetColor( Color( mMinimapBackgroundColor ).blendAlpha( mAlpha ) );
		BR->batchQuad( rect );
	}

	BR->quadsSetColor( Color( mMinimapVisibleAreaColor ).blendAlpha( mAlpha ) );
	BR->batchQuad( { { rect.Left, visibleY }, Sizef( rect.getWidth(), scrollerHeight ) } );
	if ( mMinimapHover || mMinimapDragging ) {
		BR->quadsSetColor( Color( mMinimapHoverColor ).blendAlpha( mAlpha ) );
		BR->batchQuad( { { rect.Left, visibleY }, Sizef( rect.getWidth(), scrollerHeight ) } );
	}

	Float gutterWidth = PixelDensity::dpToPx( mMinimapConfig.gutterWidth );
	Float lineY = rect.Top;

	const auto* batchSyntaxType = &SYNTAX_NORMAL;
	Color color = mColorScheme.getSyntaxStyle( *batchSyntaxType ).color;
	color.a *= 0.5f;
	Float batchWidth = 0;
	Float batchStart = rect.Left;
	Float minimapCutoffX = rect.Left + rect.getWidth();
	Float widthScale = charSpacing / getGlyphWidth();
	Int64 maxVisibleColumn = eeceil( rect.getWidth() / charSpacing );
	auto flushBatch = [this, &color, &batchSyntaxType, &batchStart, &batchWidth, &lineY, &BR,
					   &charHeight]( const SyntaxStyleType& type ) {
		Color oldColor = color;
		color = mColorScheme.getSyntaxStyle( *batchSyntaxType ).color;
		if ( color != Color::Transparent ) {
			color.a *= 0.5f;
		} else {
			color = oldColor;
		}

		if ( batchWidth > 0 ) {
			BR->quadsSetColor( color.blendAlpha( mAlpha ) );
			BR->batchQuad( { { batchStart, lineY }, { batchWidth, charHeight } } );
		}

		batchSyntaxType = &type;
		batchStart += batchWidth;
		batchWidth = 0;
	};

	int endidx = minimapStartLine + maxMinmapLines;
	endidx = eemin( endidx, lineCount - 1 );

	Float minimapStart = rect.Left + gutterWidth;

	const auto drawTextRanges = [this, &BR, &minimapStart, &charHeight, &minimapStartLine,
								 &lineSpacing, &rect, &minimapCutoffX, &widthScale, &endidx,
								 &maxVisibleColumn]( const TextRanges& ranges,
													 const Color& backgroundColor,
													 bool drawCompleteLine = false ) {
		BR->quadsSetColor( backgroundColor );
		Int64 lineSkip = -1;
		for ( const auto& range : ranges ) {
			if ( !mLineWrapping.isWrapEnabled() && range.start().column() > maxVisibleColumn )
				continue;

			auto rangeStart = mLineWrapping.getVisualLineInfo( range.start() );
			auto rangeEnd = mLineWrapping.getVisualLineInfo( range.end() );

			if ( minimapStartLine > rangeEnd.visualIndex || endidx < rangeStart.visualIndex )
				continue;

			if ( ranges.isSorted() && rangeEnd.visualIndex > endidx )
				break;

			if ( lineSkip == rangeStart.visualIndex )
				continue;

			auto selRects = getTextRangeRectangles(
				range, { 0, rect.Top - minimapStartLine * lineSpacing }, {}, lineSpacing );

			for ( auto& selRect : selRects ) {
				auto curLine = eefloor( selRect.Top / lineSpacing );

				if ( curLine == lineSkip )
					continue;

				selRect.Bottom = selRect.Top + charHeight;
				if ( drawCompleteLine ) {
					selRect.Left = minimapStart;
					selRect.Right = minimapStart + rect.getWidth();
				} else {
					selRect.Left = minimapStart + selRect.Left * widthScale;
					selRect.Right = minimapStart + selRect.Right * widthScale;
				}

				if ( selRect.Left <= minimapCutoffX ) {
					BR->batchQuad( selRect );
				} else {
					lineSkip = curLine;
				}
			}
		}
	};

	auto drawWordMatch = [this, &drawTextRanges]( const String& text, const Int64& ln ) {
		size_t pos = 0;
		const String& line( mDoc->line( ln ).getText() );
		if ( line.size() > 300 )
			return;
		do {
			pos = line.find( text, pos );
			if ( pos != String::InvalidPos ) {
				Int64 startCol = pos;
				Int64 endCol = pos + text.size();
				TextRange range( { ln, startCol }, { ln, endCol } );
				drawTextRanges( { range }, Color( mMinimapHighlightColor ).blendAlpha( mAlpha ) );
				pos = endCol;
			} else {
				break;
			}
		} while ( true );
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
			auto text( selectionLine.view().substr( selection.start().column(),
													selection.end().column() -
														selection.start().column() ) );
			if ( !text.empty() )
				selectionString = text;
		}
	}

	if ( !mHighlightWord.isEmpty() ) {
		Lock l( mHighlightWordCacheMutex );
		drawTextRanges( mHighlightWordCache, Color( mMinimapHighlightColor ).blendAlpha( mAlpha ) );
	}

	Int64 minimapStartDocLine = mLineWrapping.getDocumentLine( minimapStartLine ).line();
	Int64 endDocIdx = mLineWrapping.getDocumentLine( endidx ).line();

	for ( Int64 line = minimapStartDocLine; line <= endDocIdx; line++ ) {
		batchSyntaxType = &SYNTAX_NORMAL;
		batchStart = rect.Left + gutterWidth;
		batchWidth = 0;

		if ( mHighlightWord.isEmpty() && !selectionString.empty() )
			drawWordMatch( selectionString, line );

		for ( auto* plugin : mPlugins ) {
			plugin->minimapDrawBeforeLineText( this, line, { rect.Left, lineY },
											   { rect.getWidth(), charHeight }, charSpacing,
											   gutterWidth, drawTextRanges );
		}

		const auto& tokens = mDoc->getHighlighter()->getLine( line, false );
		const auto& text = mDoc->line( line ).getText();
		Int64 pos = 0;
		bool wrappedLine = mLineWrapping.isWrappedLine( line );

		if ( wrappedLine ) {
			bool outOfRange = false;
			auto vline = mLineWrapping.getVisualLine( line );
			size_t curvline = 1;
			Int64 nextLineCol = vline.visualLines[curvline].column();
			Int64 lineLength = text.size();
			Int64 curVisualIndex;

			for ( const auto& token : tokens ) {
				if ( outOfRange )
					break;

				if ( !token.len )
					continue;

				if ( *batchSyntaxType != token.type ) {
					flushBatch( *batchSyntaxType );
					batchSyntaxType = &token.type;
				}

				curVisualIndex = vline.visualIndex + curvline - 1;
				Int64 remainingToken = token.len;

				while ( remainingToken > 0 ) {
					Int64 maxLength = nextLineCol - pos;
					Int64 maxPos = pos + std::min( remainingToken, maxLength );

					if ( curVisualIndex >= minimapStartLine ) {
						for ( auto i = pos; i < maxPos; i++ ) {
							String::StringBaseType ch = text[i];
							if ( ch == ' ' || ch == '\n' ) {
								flushBatch( token.type );
								batchStart += charSpacing;
							} else if ( ch == '\t' ) {
								flushBatch( token.type );
								batchStart += charSpacing * mMinimapConfig.tabWidth;
							} else {
								batchWidth += charSpacing;
							}
						}
					}

					remainingToken = remainingToken - ( maxPos - pos );
					pos = maxPos;

					if ( pos == nextLineCol ) {

						if ( curVisualIndex >= minimapStartLine ) {
							flushBatch( token.type );
							lineY += lineSpacing;
						}

						curvline++;
						curVisualIndex = vline.visualIndex + curvline - 1;
						if ( curvline < vline.visualLines.size() ) {
							nextLineCol = vline.visualLines[curvline].column();
						} else {
							nextLineCol = lineLength;
						}

						batchStart = rect.Left + gutterWidth;
						batchWidth = 0;

						if ( pos == lineLength )
							break;

						if ( curVisualIndex > endidx + 1 ) {
							outOfRange = true;
							break;
						}
					} else if ( !remainingToken ) {
						break;
					}
				}
			}
		} else {
			Int64 tokenPos = 0;
			for ( const auto& token : tokens ) {
				if ( *batchSyntaxType != token.type ) {
					flushBatch( *batchSyntaxType );
					batchSyntaxType = &token.type;
				}

				size_t pos = tokenPos;
				size_t end = pos + token.len <= text.size() ? tokenPos + token.len : text.size();

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

				tokenPos += token.len;
			}
		}

		flushBatch( SYNTAX_NORMAL );

		for ( auto* plugin : mPlugins ) {
			plugin->minimapDrawAfterLineText( this, line, { rect.Left, lineY },
											  { rect.getWidth(), charHeight }, charSpacing,
											  gutterWidth, drawTextRanges );
		}

		if ( !wrappedLine )
			lineY += lineSpacing;
	}

	if ( mHighlightTextRange.isValid() && mHighlightTextRange.hasSelection() ) {
		drawTextRanges( { mHighlightTextRange },
						Color( mMinimapSelectionColor ).blendAlpha( mAlpha ) );
	}

	if ( mDoc->hasSelection() ) {
		drawTextRanges( mDoc->getSelectionsSorted(),
						Color( mMinimapSelectionColor ).blendAlpha( mAlpha ) );
	}

	for ( size_t i = 0; i < mDoc->getSelections().size(); ++i ) {
		const auto& selection = mDoc->getSelectionIndex( i );
		Float selectionY =
			rect.Top + ( mLineWrapping.getVisualLineInfo( selection.start() ).visualIndex -
						 minimapStartLine ) *
						   lineSpacing;
		BR->quadsSetColor( Color( mMinimapCurrentLineColor ).blendAlpha( mAlpha ) );
		BR->batchQuad( { { rect.Left, selectionY }, { rect.getWidth(), lineSpacing } } );
	}

	BR->draw();
}

Vector2f UICodeEditor::getScreenStart() const {
	return Vector2f( eefloor( mScreenPos.x + mPaddingPx.Left ),
					 eefloor( mScreenPos.y + mPaddingPx.Top ) );
}

Vector2f UICodeEditor::getScreenScroll() const {
	Float gutterWidth = getGutterWidth();
	Vector2f screenStart( getScreenStart() );
	Vector2f start( screenStart.x + gutterWidth, screenStart.y + getPluginsTopSpace() );
	Vector2f startScroll( start - mScroll );
	return startScroll;
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

static bool isAlreadyClosedTag( TextDocument* doc, TextPosition start,
								size_t maxSearchTokens = 1000 ) {
	SyntaxHighlighter* highlighter = doc->getHighlighter();
	TextPosition endOfDoc = doc->endOfDoc();
	do {
		String::StringBaseType ch = doc->getChar( start );
		switch ( ch ) {
			case '>': {
				auto tokenType = highlighter->getTokenTypeAt( start );
				if ( tokenType != "comment"_sst && tokenType != "string"_sst )
					return true;
				break;
			}
			case '<': {
				auto tokenType = highlighter->getTokenTypeAt( start );
				if ( tokenType != "comment"_sst && tokenType != "string"_sst )
					return false;
				break;
			}
			default:
				break;
		}
		start = doc->positionOffset( start, 1 );
	} while ( start.isValid() && start != endOfDoc && --maxSearchTokens );
	return false;
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
	SyntaxStyleType type = mDoc->getHighlighter()->getTokenTypeAt( start );
	if ( type == SyntaxStyleTypes::String || type == SyntaxStyleTypes::Comment )
		return false;
	size_t foundOpenPos = line.find_last_of( "<", start.column() - 1 );
	if ( foundOpenPos == String::InvalidPos || start.column() - foundOpenPos < 1 )
		return false;
	std::string tag( line.substr( foundOpenPos, start.column() - foundOpenPos ).toUtf8() );
	LuaPattern pattern( "<([%w_%-]+).*>" );
	auto match = pattern.gmatch( tag );
	if ( match.matches() && !isAlreadyClosedTag( mDoc.get(), start ) ) {
		mDoc->textInput( String( "</" + match.group( 1 ) + ">" ) );
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
