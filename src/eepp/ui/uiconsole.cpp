#include <algorithm>
#include <eepp/audio/listener.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiconsole.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

using namespace EE::Window;
using namespace EE::Scene;

namespace EE { namespace UI {

UIConsole* UIConsole::New() {
	return eeNew( UIConsole, ( nullptr, true, true, 8192 ) );
}

UIConsole* UIConsole::NewOpt( Font* font, const bool& makeDefaultCommands, const bool& attachToLog,
							  const unsigned int& maxLogLines ) {
	return eeNew( UIConsole, ( font, makeDefaultCommands, attachToLog, maxLogLines ) );
}

UIConsole::UIConsole( Font* font, const bool& makeDefaultCommands, const bool& attachToLog,
					  const unsigned int& maxLogLines ) :
	UIWidget( "console" ), mKeyBindings( getUISceneNode()->getWindow()->getInput() ) {
	setFlags( UI_AUTO_PADDING );
	mFlags |= UI_TAB_STOP | UI_SCROLLABLE | UI_TEXT_SELECTION_ENABLED;
	setClipType( ClipType::ContentBox );

	setBackgroundColor( 0x201F1FEE );

	mDoc.registerClient( this );
	registerCommands();
	registerKeybindings();

	mFontStyleConfig.Font = font;
	if ( nullptr == font )
		mFontStyleConfig.Font = FontManager::instance()->getByName( "monospace" );

	mMaxLogLines = maxLogLines;

	if ( nullptr == mFontStyleConfig.Font )
		Log::error( "A monospace font must be loaded to be able to use the console.\nTry loading "
					"a font with the name \"monospace\"" );

	setFontSelectionBackColor( 0x4d668066 );

	if ( makeDefaultCommands )
		createDefaultCommands();

	mTextCache.resize( maxLinesOnScreen() );

	cmdGetLog();

	if ( attachToLog )
		Log::instance()->addLogReader( this );

	applyDefaultTheme();

	subscribeScheduledUpdate();
}

UIConsole::~UIConsole() {
	if ( Log::existsSingleton() )
		Log::instance()->removeLogReader( this );
}

Uint32 UIConsole::getType() const {
	return UI_TYPE_CONSOLE;
}

bool UIConsole::isType( const Uint32& type ) const {
	return UIConsole::getType() == type ? true : UIWidget::isType( type );
}

void UIConsole::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "console" );

	onThemeLoaded();
}

void UIConsole::scheduledUpdate( const Time& ) {
	if ( mMouseDown ) {
		if ( !( getUISceneNode()->getWindow()->getInput()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
			mMouseDown = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		} else {
			onMouseDown( getUISceneNode()->getEventDispatcher()->getMousePos(),
						 getUISceneNode()->getEventDispatcher()->getPressTrigger() );
		}
	}

	if ( hasFocus() && getUISceneNode()->getWindow()->hasFocus() ) {
		if ( mBlinkTime != Time::Zero && mBlinkTimer.getElapsedTime() > mBlinkTime ) {
			mCursorVisible = !mCursorVisible;
			mBlinkTimer.restart();
			invalidateDraw();
		}
	}
}

const Time& UIConsole::getBlinkTime() const {
	return mBlinkTime;
}

void UIConsole::setBlinkTime( const Time& blinkTime ) {
	if ( blinkTime != mBlinkTime ) {
		mBlinkTime = blinkTime;
		resetCursor();
		if ( mBlinkTime == Time::Zero )
			mCursorVisible = true;
	}
}

size_t UIConsole::getMenuIconSize() const {
	return mMenuIconSize;
}

void UIConsole::setMenuIconSize( size_t menuIconSize ) {
	mMenuIconSize = menuIconSize;
}

KeyBindings& UIConsole::getKeyBindings() {
	return mKeyBindings;
}

TextDocument& UIConsole::getDoc() {
	return mDoc;
}

Font* UIConsole::getFont() const {
	return mFontStyleConfig.Font;
}

const UIFontStyleConfig& UIConsole::getFontStyleConfig() const {
	return mFontStyleConfig;
}

UIConsole* UIConsole::setFont( Font* font ) {
	if ( mFontStyleConfig.Font != font ) {
		mFontStyleConfig.Font = font;
		invalidateDraw();
		onFontChanged();
	}
	return this;
}

bool UIConsole::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
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
			mFontStyleConfig.FontSelectedColor = attribute.asColor();
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
			setFontOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			setFontOutlineColor( attribute.asColor() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::string UIConsole::getPropertyString( const PropertyDefinition* propertyDef,
										  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
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
			return Text::styleFlagToString( getFontStyleConfig().getFontStyle() );
		case PropertyId::TextStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getFontOutlineThickness() ), "px" );
		case PropertyId::TextStrokeColor:
			return getFontOutlineColor().toHexString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIConsole::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = {
		PropertyId::Color,			PropertyId::TextShadowColor,	PropertyId::TextShadowOffset,
		PropertyId::SelectionColor, PropertyId::SelectionBackColor, PropertyId::FontFamily,
		PropertyId::FontSize,		PropertyId::FontStyle,			PropertyId::TextStrokeWidth,
		PropertyId::TextStrokeColor };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

UIConsole* UIConsole::setFontSize( const Float& size ) {
	if ( mFontStyleConfig.CharacterSize != size ) {
		mFontStyleConfig.CharacterSize =
			eeabs( size - (int)size ) == 0.5f || (int)size == size ? size : eefloor( size );
		invalidateDraw();
		onFontChanged();
	}
	return this;
}

const Float& UIConsole::getFontSize() const {
	return mFontStyleConfig.getFontCharacterSize();
}

UIConsole* UIConsole::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.getFontColor() != color ) {
		mFontStyleConfig.FontColor = color;
		invalidateDraw();
		onFontStyleChanged();
	}
	return this;
}

const Color& UIConsole::getFontColor() const {
	return mFontStyleConfig.getFontColor();
}

const Color& UIConsole::getFontSelectedColor() const {
	return mFontStyleConfig.getFontSelectedColor();
}

UIConsole* UIConsole::setFontSelectedColor( const Color& color ) {
	if ( mFontStyleConfig.getFontSelectedColor() != color ) {
		mFontStyleConfig.FontSelectedColor = color;
		invalidateDraw();
		onFontStyleChanged();
	}
	return this;
}

UIConsole* UIConsole::setFontSelectionBackColor( const Color& color ) {
	if ( mFontStyleConfig.getFontSelectionBackColor() != color ) {
		mFontStyleConfig.FontSelectionBackColor = color;
		invalidateDraw();
		onFontStyleChanged();
	}
	return this;
}

const Color& UIConsole::getFontSelectionBackColor() const {
	return mFontStyleConfig.getFontSelectionBackColor();
}

UIConsole* UIConsole::setFontShadowColor( const Color& color ) {
	if ( color != mFontStyleConfig.getFontShadowColor() ) {
		mFontStyleConfig.ShadowColor = color;
		if ( mFontStyleConfig.ShadowColor != Color::Transparent )
			mFontStyleConfig.Style |= Text::Shadow;
		else
			mFontStyleConfig.Style &= ~Text::Shadow;
		onFontStyleChanged();
	}
	return this;
}

const Color& UIConsole::getFontShadowColor() const {
	return mFontStyleConfig.ShadowColor;
}

UIConsole* UIConsole::setFontShadowOffset( const Vector2f& offset ) {
	if ( offset != mFontStyleConfig.getFontShadowOffset() ) {
		mFontStyleConfig.ShadowOffset = offset;
		onFontStyleChanged();
	}
	return this;
}

const Vector2f& UIConsole::getFontShadowOffset() const {
	return mFontStyleConfig.ShadowOffset;
}

UIConsole* UIConsole::setFontStyle( const Uint32& fontStyle ) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mFontStyleConfig.Style = fontStyle;
		onFontStyleChanged();
	}
	return this;
}

UIConsole* UIConsole::setFontOutlineThickness( const Float& outlineThickness ) {
	if ( mFontStyleConfig.OutlineThickness != outlineThickness ) {
		mFontStyleConfig.OutlineThickness = outlineThickness;
		onFontStyleChanged();
	}
	return this;
}

const Float& UIConsole::getFontOutlineThickness() const {
	return mFontStyleConfig.OutlineThickness;
}

UIConsole* UIConsole::setFontOutlineColor( const Color& outlineColor ) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mFontStyleConfig.OutlineColor = outlineColor;
		onFontStyleChanged();
	}
	return this;
}

const Color& UIConsole::getFontOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

void UIConsole::onFontChanged() {
	updateCacheSize();
}

void UIConsole::onFontStyleChanged() {
	onFontChanged();
}

void UIConsole::addCommand( const std::string& command, const ConsoleCallback& cb ) {
	if ( !( mCallbacks.count( command ) > 0 ) )
		mCallbacks[command] = cb;
}

void UIConsole::setCommand( const std::string& command, const ConsoleCallback& cb ) {
	mCallbacks[command] = cb;
}

const Uint32& UIConsole::getMaxLogLines() const {
	return mMaxLogLines;
}

void UIConsole::setMaxLogLines( const Uint32& maxLogLines ) {
	mMaxLogLines = maxLogLines;
}

void UIConsole::privPushText( String&& str ) {
	Lock l( mMutex );
	mCmdLog.push_back( { std::move( str ), String::hash( str ) } );
	if ( mVisible )
		invalidateDraw();
	if ( mCmdLog.size() >= mMaxLogLines )
		mCmdLog.pop_front();
}

Int32 UIConsole::linesOnScreen() {
	auto lh = getLineHeight();
	if ( lh == 0.f )
		return 0;
	return static_cast<Int32>(
		( ( getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom ) / lh ) - 1 );
}

Int32 UIConsole::maxLinesOnScreen() {
	auto lh = getLineHeight();
	if ( lh == 0.f )
		return 1;
	auto maxLines =
		( ( getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom ) / lh ) + 3;
	return static_cast<Int32>( eemax( 1.f, maxLines ) );
}

void UIConsole::draw() {
	if ( !mVisible || NULL == mFontStyleConfig.Font )
		return;

	Lock l( mMutex );
	Int32 linesInScreen = linesOnScreen();
	size_t pos = 0;
	Float curY;
	Float lineHeight = getLineHeight();
	Float cw = mFontStyleConfig.Font->getGlyph( '_', mFontStyleConfig.CharacterSize, false, false )
				   .advance;

	mCon.min = eemax( 0, (Int32)mCmdLog.size() - linesInScreen );
	mCon.max = (int)mCmdLog.size() - 1;

	UIWidget::draw();

	Color fontColor( Color( mFontStyleConfig.FontColor.r, mFontStyleConfig.FontColor.g,
							mFontStyleConfig.FontColor.b )
						 .blendAlpha( (Uint8)mAlpha ) );

	Primitives p;
	p.setColor( Color( mFontStyleConfig.FontSelectionBackColor ).blendAlpha( (Uint8)mAlpha ) );
	auto to = eemax( mCon.min - mCon.modif, 0 );
	auto from = eemin( mCon.max - mCon.modif, (int)mCmdLog.size() - 1 );

	for ( int i = from; i >= to; i-- ) {
		curY = mScreenPos.y + getPixelsSize().getHeight() - mPaddingPx.Bottom - pos * lineHeight -
			   lineHeight * 2 - 1;

		auto selNorm = mSelection.normalized();
		if ( mSelection.isValid() && mSelection.hasSelection() && selNorm.containsLine( i ) &&
			 i < (int)mCmdLog.size() ) {
			auto startCol = eemin( (Int64)mCmdLog[i].log.size(), selNorm.start().column() );
			auto endCol = eemin( (Int64)mCmdLog[i].log.size(), selNorm.end().column() );

			if ( i == selNorm.start().line() ) {
				auto tsubstr = mCmdLog[i].log.view().substr(
					startCol, selNorm.end().line() == i ? eemax( (Int64)0, endCol - startCol )
														: (Int64)mCmdLog[i].log.size() - startCol );
				auto twidth =
					Text::getTextWidth( mFontStyleConfig.Font, mFontStyleConfig.CharacterSize,
										tsubstr, mFontStyleConfig.Style );
				auto fsubstr = mCmdLog[i].log.view().substr( 0, startCol );
				auto fwidth =
					Text::getTextWidth( mFontStyleConfig.Font, mFontStyleConfig.CharacterSize,
										fsubstr, mFontStyleConfig.Style );
				p.drawRectangle( Rectf( { mScreenPos.x + mPaddingPx.Left + fwidth, curY },
										{ twidth, lineHeight } ) );
			} else if ( i == selNorm.end().line() ) {
				auto fsubstr = mCmdLog[i].log.view().substr( 0, endCol );
				auto fwidth =
					Text::getTextWidth( mFontStyleConfig.Font, mFontStyleConfig.CharacterSize,
										fsubstr, mFontStyleConfig.Style );
				p.drawRectangle(
					Rectf( { mScreenPos.x + mPaddingPx.Left, curY }, { fwidth, lineHeight } ) );
			} else {
				p.drawRectangle(
					Rectf( { mScreenPos.x + mPaddingPx.Left, curY },
						   { getPixelsSize().getWidth() - mPadding.getWidth(), lineHeight } ) );
			}

			p.setForceDraw( true );
		}

		Text& text = mTextCache[pos].text;
		text.setStyleConfig( mFontStyleConfig );
		text.setFillColor( fontColor );
		if ( mCmdLog[i].hash != mTextCache[pos].hash ) {
			if ( mCmdLog[i].log.size() * cw <= mSize.getWidth() ) {
				text.setString( mCmdLog[i].log );
				mTextCache[pos].hash = mCmdLog[i].hash;
			} else {
				auto substr = mCmdLog[i].log.substr( 0, ( mSize.getWidth() + 8 * cw ) / cw );
				mTextCache[pos].hash = String::hash( substr );
				text.setString( substr );
			}
		}
		text.draw( mScreenPos.x + mPaddingPx.Left, curY );
		pos++;
	}

	curY = mScreenPos.y + getPixelsSize().getHeight() - mPaddingPx.Bottom - lineHeight - 1;

	auto editCharWidth = Text::getTextWidth( String( "> " ), mFontStyleConfig );

	if ( mDoc.hasSelection() ) {
		Float selStartPos =
			editCharWidth + Text::getTextWidth( mDoc.getCurrentLine().getText().view().substr(
													0, mDoc.getSelection( true ).start().column() ),
												mFontStyleConfig );
		Float selWidth = Text::getTextWidth( mDoc.getSelectedText(), mFontStyleConfig );
		p.drawRectangle( Rectf( { mScreenPos.x + mPaddingPx.Left + selStartPos, curY },
								{ selWidth, lineHeight } ) );
	}

	Text& text = mTextCache[mTextCache.size() - 1].text;
	text.setStyleConfig( mFontStyleConfig );
	text.setFillColor( fontColor );
	text.setString( "> " + mDoc.getCurrentLine().getTextWithoutNewLine() );
	text.draw( mScreenPos.x + mPaddingPx.Left, curY );

	if ( mCursorVisible ) {
		Float cursorPos =
			editCharWidth + Text::getTextWidth( mDoc.getCurrentLine().getText().view().substr(
													0, mDoc.getSelection().start().column() ),
												mFontStyleConfig );
		Rectf r( { mScreenPos.x + mPaddingPx.Left + cursorPos, curY }, { cursorPos, lineHeight } );
		updateIMELocation( r );
		if ( hasFocus() && getUISceneNode()->getWindow()->getIME().isEditing() ) {
			FontStyleConfig config( mFontStyleConfig );
			config.FontColor = mFontStyleConfig.getFontSelectedColor();
			getUISceneNode()->getWindow()->getIME().draw( r.getPosition(), getLineHeight(),
														  mFontStyleConfig,
														  Color( fontColor ).blendAlpha( mAlpha ) );
		} else {
			Text& text2 = mTextCache[mTextCache.size() - 2].text;
			text2.setStyleConfig( mFontStyleConfig );
			text2.setFillColor( fontColor );
			text2.setString( "_" );
			text2.draw( r.Left, r.Top );
		}
	}

	if ( mShowFps ) {
		Float cw =
			mFontStyleConfig.Font->getGlyph( '_', mFontStyleConfig.CharacterSize, false, false )
				.advance;
		Text& text = mTextCache[mTextCache.size() - 3].text;
		Color OldColor1( text.getColor() );
		text.setStyleConfig( mFontStyleConfig );
		text.setFillColor( fontColor );
		text.setString( "FPS: " + String::toString( getUISceneNode()->getWindow()->getFPS() ) );
		text.draw( mScreenPos.x + getPixelsSize().getWidth() - text.getTextWidth() - cw -
					   mPaddingPx.Right,
				   mScreenPos.y + mPaddingPx.Top + eefloor( lineHeight / 2 ) );
		text.setFillColor( OldColor1 );
	}
}

// CMDS
void UIConsole::createDefaultCommands() {
	addCommand( "clear", [this]( const auto& ) { cmdClear(); } );
	addCommand( "quit", [this]( const auto& ) { getUISceneNode()->getWindow()->close(); } );
	addCommand( "cmdlist", [this]( const auto& ) { cmdCmdList(); } );
	addCommand( "help", [this]( const auto& ) { cmdCmdList(); } );
	addCommand( "showcursor", [this]( const auto& params ) { cmdShowCursor( params ); } );
	addCommand( "setfpslimit", [this]( const auto& params ) { cmdFrameLimit( params ); } );
	addCommand( "getlog", [this]( const auto& ) { cmdGetLog(); } );
	addCommand( "setgamma", [this]( const auto& params ) { cmdSetGamma( params ); } );
	addCommand( "setvolume", [this]( const auto& params ) { cmdSetVolume( params ); } );
	addCommand( "getgpuextensions", [this]( const auto& ) { cmdGetGpuExtensions(); } );
	addCommand( "dir", [this]( const auto& params ) { cmdDir( params ); } );
	addCommand( "ls", [this]( const auto& params ) { cmdDir( params ); } );
	addCommand( "showfps", [this]( const auto& params ) { cmdShowFps( params ); } );
	addCommand( "gettexturememory", [this]( const auto& ) { cmdGetTextureMemory(); } );
	addCommand( "hide", [this]( const auto& ) { hide(); } );
	addCommand( "grep", [this]( const auto& params ) { cmdGrep( params ); } );
	addCommand( "arch", [this]( const auto& ) { privPushText( Sys::getOSArchitecture() ); } );
	addCommand( "pwd", [this]( const auto& ) {
		privPushText( FileSystem::getCurrentWorkingDirectory() );
	} );
	addCommand( "env", [this]( const auto& ) {
		auto envVars = Sys::getEnvironmentVariables();
		for ( const auto& env : envVars )
			privPushText( env.first + "=" + env.second );
	} );
	addCommand( "exec", [this]( const std::vector<String>& params ) {
		auto executeArr = params;
		executeArr.erase( executeArr.begin() );
		std::string execute = String::join( executeArr );
		Process p;
		p.create( execute, Process::CombinedStdoutStderr | Process::getDefaultOptions() );
		std::string buffer;
		p.readAllStdOut( buffer, Seconds( 1 ) );
		auto lines = String::split( buffer );
		for ( const auto& line : lines )
			privPushText( line );
	} );
}

void UIConsole::cmdClear() {
	size_t cutLines = getPixelsSize().getHeight() / mFontStyleConfig.CharacterSize;
	for ( size_t i = 0; i < cutLines; i++ )
		privPushText( "" );
}

void UIConsole::cmdGetTextureMemory() {
	privPushText( "Total texture memory used: " +
				  FileSystem::sizeToString( TextureFactory::instance()->getTextureMemorySize() ) );
}

void UIConsole::cmdCmdList() {
	for ( auto itr = mCallbacks.begin(); itr != mCallbacks.end(); ++itr )
		privPushText( "\t" + itr->first );
}

void UIConsole::cmdShowCursor( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			getUISceneNode()->getWindow()->getCursorManager()->setVisible( 0 != tInt );
		} else
			privPushText( "Valid parameters are 0 or 1." );
	} else {
		privPushText( "No parameters. Valid parameters are 0 ( hide ) or 1 ( show )." );
	}
}

void UIConsole::cmdFrameLimit( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt >= 0 && tInt <= 10000 ) ) {
			getUISceneNode()->getWindow()->setFrameRateLimit( tInt );
			return;
		}
	}

	privPushText( "Valid parameters are between 0 and 10000 (0 = no limit)." );
}

void UIConsole::cmdGetLog() {
	std::vector<String> tvec =
		String::split( String( String::toString( Log::instance()->getBuffer() ) ) );
	if ( tvec.size() > 0 ) {
		for ( unsigned int i = 0; i < tvec.size(); i++ )
			privPushText( std::move( tvec[i] ) );
	}
}

void UIConsole::cmdGetGpuExtensions() {
	std::vector<String> tvec = String::split( String( GLi->getExtensions() ), ' ' );
	if ( tvec.size() > 0 ) {
		for ( unsigned int i = 0; i < tvec.size(); i++ )
			privPushText( std::move( tvec[i] ) );
	}
}

void UIConsole::cmdGrep( const std::vector<String>& params ) {
	if ( params.empty() )
		return;
	bool caseSensitive = !std::any_of( params.begin(), params.end(),
									   []( const auto& other ) { return "-i" == other; } );
	String search = params[params.size() - 1];

	if ( caseSensitive ) {
		for ( const auto& cmd : mCmdLog )
			if ( !cmd.log.empty() && cmd.log[0] != '>' && String::contains( cmd.log, search ) )
				privPushText( String( cmd.log ) );
	} else {
		search.toLower();
		for ( const auto& cmd : mCmdLog )
			if ( !cmd.log.empty() && cmd.log[0] != '>' &&
				 String::contains( String::toLower( cmd.log ), search ) )
				privPushText( String( cmd.log ) );
	}
}

void UIConsole::cmdSetGamma( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0.f;
		bool Res = String::fromString<Float>( tFloat, params[1] );

		if ( Res && ( tFloat > 0.1f && tFloat <= 10.0f ) ) {
			getUISceneNode()->getWindow()->setGamma( tFloat, tFloat, tFloat );
			return;
		}
	}

	privPushText( "Valid parameters are between 0.1 and 10." );
}

void UIConsole::cmdSetVolume( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0.f;

		bool Res = String::fromString<Float>( tFloat, params[1] );

		if ( Res && ( tFloat >= 0.0f && tFloat <= 100.0f ) ) {
			EE::Audio::Listener::setGlobalVolume( tFloat );
			return;
		}
	}

	privPushText( "Valid parameters are between 0 and 100." );
}

void UIConsole::cmdDir( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		String Slash( FileSystem::getOSSlash() );
		String myPath = params[1];
		String myOrder;

		if ( params.size() > 2 ) {
			myOrder = params[2];
		}

		if ( FileSystem::isDirectory( myPath ) ) {
			unsigned int i;

			std::vector<String> mFiles = FileSystem::filesGetInPath( myPath );
			std::sort( mFiles.begin(), mFiles.end() );

			privPushText( "Directory: " + myPath );

			if ( myOrder == "ff" ) {
				std::vector<String> mFolders;
				std::vector<String> mFile;

				for ( i = 0; i < mFiles.size(); i++ ) {
					if ( FileSystem::isDirectory( myPath + Slash + mFiles[i] ) ) {
						mFolders.push_back( mFiles[i] );
					} else {
						mFile.push_back( mFiles[i] );
					}
				}

				if ( mFolders.size() )
					privPushText( "Folders: " );

				for ( i = 0; i < mFolders.size(); i++ )
					privPushText( "	" + mFolders[i] );

				if ( mFolders.size() )
					privPushText( "Files: " );

				for ( i = 0; i < mFile.size(); i++ )
					privPushText( "	" + mFile[i] );

			} else {
				for ( i = 0; i < mFiles.size(); i++ )
					privPushText( "	" + mFiles[i] );
			}
		} else {
			if ( myPath == "help" )
				privPushText(
					"You can use a third parameter to show folders first, the parameter is ff." );
			else
				privPushText( "Path \"" + myPath + "\" is not a directory." );
		}
	} else {
		privPushText( "Expected a path to list. Example of usage: ls /home" );
	}
}

void UIConsole::cmdShowFps( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool res = String::fromString<Int32>( tInt, params[1] );

		if ( res && ( tInt == 0 || tInt == 1 ) ) {
			mShowFps = 0 != tInt;
			return;
		}
	}

	privPushText( "Valid parameters are 0 ( hide ) or 1 ( show )." );
}

void UIConsole::writeLog( const std::string_view& text ) {
	std::vector<std::string_view> strings = String::split( text );
	for ( size_t i = 0; i < strings.size(); i++ )
		privPushText( strings[i] );
}

const bool& UIConsole::isShowingFps() const {
	return mShowFps;
}

void UIConsole::showFps( const bool& show ) {
	mShowFps = show;
}

void UIConsole::copy() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText().toUtf8() );
}

void UIConsole::cut() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText().toUtf8() );
	mDoc.deleteSelection();
}

bool UIConsole::getEscapePastedText() const {
	return mEscapePastedText;
}

void UIConsole::setEscapePastedText( bool escapePastedText ) {
	mEscapePastedText = escapePastedText;
}

void UIConsole::paste() {
	String pasted( getUISceneNode()->getWindow()->getClipboard()->getText() );
	if ( mEscapePastedText ) {
		pasted.escape();
	} else {
		String::replaceAll( pasted, "\n", "" );
	}
	mDoc.textInput( pasted );
	sendCommonEvent( Event::OnTextPasted );
}

Uint32 UIConsole::onKeyDown( const KeyEvent& event ) {
	if ( getUISceneNode()->getWindow()->getIME().isEditing() )
		return 0;

	if ( ( event.getKeyCode() == KEY_TAB ) &&
		 mDoc.getSelection().start().column() == (Int64)mDoc.getCurrentLine().size() - 1 ) {
		printCommandsStartingWith( mDoc.getCurrentLine().getTextWithoutNewLine() );
		getFilesFrom( mDoc.getCurrentLine().getTextWithoutNewLine().toUtf8(),
					  mDoc.getSelection().start().column() );
		return 1;
	}

	if ( event.getMod() & KEYMOD_SHIFT ) {
		if ( event.getKeyCode() == KEY_UP && mCon.min - mCon.modif > 0 ) {
			mCon.modif++;
			invalidateDraw();
			return 1;
		}

		if ( event.getKeyCode() == KEY_DOWN && mCon.modif > 0 ) {
			mCon.modif--;
			invalidateDraw();
			return 1;
		}

		if ( event.getKeyCode() == KEY_HOME ) {
			size_t size;
			{
				Lock l( mMutex );
				size = mCmdLog.size();
			}
			if ( static_cast<Int32>( size ) > linesOnScreen() ) {
				mCon.modif = mCon.min;
				invalidateDraw();
				return 1;
			}
		}

		if ( event.getKeyCode() == KEY_END ) {
			mCon.modif = 0;
			invalidateDraw();
			return 1;
		}

		if ( event.getKeyCode() == KEY_PAGEUP ) {
			if ( mCon.min - mCon.modif - linesOnScreen() / 2 > 0 )
				mCon.modif += linesOnScreen() / 2;
			else
				mCon.modif = mCon.min;
			invalidateDraw();
			return 1;
		}

		if ( event.getKeyCode() == KEY_PAGEDOWN ) {
			if ( mCon.modif - linesOnScreen() / 2 > 0 )
				mCon.modif -= linesOnScreen() / 2;
			else
				mCon.modif = 0;
			invalidateDraw();
			return 1;
		}
	} else {
		if ( mLastCommands.size() > 0 ) {
			if ( event.getKeyCode() == KEY_UP && mLastLogPos > 0 ) {
				mLastLogPos--;
			}

			if ( event.getKeyCode() == KEY_DOWN &&
				 mLastLogPos < static_cast<int>( mLastCommands.size() ) ) {
				mLastLogPos++;
			}

			if ( event.getKeyCode() == KEY_UP || event.getKeyCode() == KEY_DOWN ) {
				if ( mLastLogPos == static_cast<int>( mLastCommands.size() ) ) {
					mDoc.replaceCurrentLine( "" );
				} else {
					mDoc.replaceCurrentLine( mLastCommands[mLastLogPos] );
					mDoc.moveToEndOfLine();
				}
				invalidateDraw();
				return 1;
			}
		}
	}

	std::string cmd = mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( !cmd.empty() ) {
		mDoc.execute( cmd );
		mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();
		return 1;
	}
	return UIWidget::onKeyDown( event );
}

Uint32 UIConsole::onTextInput( const TextInputEvent& event ) {
	Input* input = getUISceneNode()->getWindow()->getInput();

	if ( ( input->isLeftAltPressed() && !event.getText().empty() && event.getText()[0] == '\t' ) ||
		 ( input->isLeftControlPressed() && !input->isLeftAltPressed() &&
		   !input->isAltGrPressed() ) ||
		 input->isMetaPressed() || ( input->isLeftAltPressed() && !input->isLeftControlPressed() ) )
		return 0;

	if ( mLastExecuteEventId == getUISceneNode()->getWindow()->getInput()->getEventsSentId() )
		return 0;

	const String& text = event.getText();

	for ( size_t i = 0; i < text.size(); i++ ) {
		if ( text[i] == '\n' )
			return 0;
	}

	mDoc.textInput( text );
	invalidateDraw();
	return 1;
}

Uint32 UIConsole::onTextEditing( const TextEditingEvent& event ) {
	UIWidget::onTextEditing( event );
	mDoc.imeTextEditing( event.getText() );
	invalidateDraw();
	return 1;
}

void UIConsole::updateIMELocation( const Rectf& loc ) {
	if ( mDoc.getActiveClient() != this )
		return;
	getUISceneNode()->getWindow()->getIME().setLocation( loc.asInt() );
}

Uint32 UIConsole::onPressEnter() {
	processLine();
	sendCommonEvent( Event::OnPressEnter );
	invalidateDraw();
	return 0;
}

void UIConsole::registerCommands() {
	mDoc.setCommand( "copy", [this] {
		if ( mSelection.hasSelection() ) {
			copySelection();
		} else {
			copy();
		}
	} );
	mDoc.setCommand( "cut", [this] { cut(); } );
	mDoc.setCommand( "paste", [this] { paste(); } );
	mDoc.setCommand( "press-enter", [this] { onPressEnter(); } );
}

void UIConsole::registerKeybindings() {
	mKeyBindings.addKeybinds( {
		{ { KEY_BACKSPACE, KeyMod::getDefaultModifier() }, "delete-to-previous-word" },
		{ { KEY_BACKSPACE, KEYMOD_SHIFT }, "delete-to-previous-char" },
		{ { KEY_BACKSPACE, 0 }, "delete-to-previous-char" },
		{ { KEY_DELETE, KeyMod::getDefaultModifier() }, "delete-to-next-word" },
		{ { KEY_DELETE, 0 }, "delete-to-next-char" },
		{ { KEY_KP_ENTER, 0 }, "press-enter" },
		{ { KEY_RETURN, 0 }, "press-enter" },
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
		{ { KEY_Y, KeyMod::getDefaultModifier() }, "redo" },
		{ { KEY_Z, KeyMod::getDefaultModifier() }, "undo" },
		{ { KEY_C, KeyMod::getDefaultModifier() }, "copy" },
		{ { KEY_X, KeyMod::getDefaultModifier() }, "cut" },
		{ { KEY_V, KeyMod::getDefaultModifier() }, "paste" },
		{ { KEY_A, KeyMod::getDefaultModifier() }, "select-all" },
	} );
}

void UIConsole::resetCursor() {
	mCursorVisible = true;
	mBlinkTimer.restart();
}

Uint32 UIConsole::onFocus( NodeFocusReason reason ) {
	UIWidget::onFocus( reason );

	resetCursor();

	getSceneNode()->getWindow()->startTextInput();

	mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();

	return 1;
}

Uint32 UIConsole::onFocusLoss() {
	getSceneNode()->getWindow()->stopTextInput();
	mCursorVisible = false;
	invalidateDraw();
	return UIWidget::onFocusLoss();
}

bool UIConsole::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

TextPosition UIConsole::getPositionOnScreen( Vector2f position ) {
	convertToNodeSpace( position );
	Float lineHeight = getLineHeight();
	auto linesInScreen = linesOnScreen();
	auto firstVisibleLine = eemax( mCon.min - mCon.modif, 0 );
	Float startOffset = getPixelsSize().getHeight() - mPaddingPx.Bottom -
						linesInScreen * lineHeight +
						( linesInScreen > (Int64)mCmdLog.size()
							  ? lineHeight * ( linesInScreen - (Float)mCmdLog.size() )
							  : 0.f );
	Int64 line = eeclamp( (Int64)eefloor( ( position.y - startOffset ) / lineHeight + 1 ), (Int64)0,
						  (Int64)mCmdLog.size() - 1 );
	Int64 fline = eeclamp( firstVisibleLine + line, (Int64)0, (Int64)mCmdLog.size() - 1 );
	Int64 col = Text::findCharacterFromPos(
		{ (int)eefloor( position.x - mPaddingPx.Left ), 0 }, true, mFontStyleConfig.Font,
		mFontStyleConfig.CharacterSize, mCmdLog[fline].log, mFontStyleConfig.Style );
	return { fline, col };
}

Uint32 UIConsole::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	UIWidget::onMouseDown( position, flags );

	if ( NULL != getEventDispatcher() && isTextSelectionEnabled() && ( flags & EE_BUTTON_LMASK ) &&
		 getEventDispatcher()->getMouseDownNode() == this && !mMouseDown ) {
		getUISceneNode()->getWindow()->getInput()->captureMouse( true );
		mMouseDown = true;
		auto pos = getPositionOnScreen( position.asFloat() );
		auto prevSelection = mSelection;
		mSelection = { pos, pos };
		if ( prevSelection != mSelection )
			invalidateDraw();
	}

	return 1;
}

Uint32 UIConsole::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	UIWidget::onMouseMove( position, flags );

	if ( ( flags & EE_BUTTON_LMASK ) && getEventDispatcher()->getMouseDownNode() == this &&
		 mMouseDown ) {
		auto prevSelection = mSelection;
		mSelection.setEnd( getPositionOnScreen( position.asFloat() ) );
		if ( prevSelection != mSelection )
			invalidateDraw();
	}

	return 1;
}

static constexpr char DEFAULT_NON_WORD_CHARS[] = " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-";

Uint32 UIConsole::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	UIWidget::onMouseDoubleClick( position, flags );

	if ( ( flags & EE_BUTTON_LMASK ) ) {
		auto pos = getPositionOnScreen( position.asFloat() );
		if ( pos.line() < (Int64)mCmdLog.size() ) {
			const String& str = mCmdLog[pos.line()].log;
			if ( pos.column() < (Int64)str.size() ) {
				auto start = str.find_last_of( DEFAULT_NON_WORD_CHARS, pos.column() );
				auto end = str.find_first_of( DEFAULT_NON_WORD_CHARS, pos.column() );
				if ( start == String::InvalidPos )
					start = 0;
				else
					start = eemin( start + 1, str.size() - 1 );
				if ( end == String::InvalidPos )
					end = str.size();
				auto prevSelection = mSelection;
				mSelection = { { pos.line(), static_cast<Int64>( start ) },
							   { pos.line(), static_cast<Int64>( end ) } };
				if ( prevSelection != mSelection )
					invalidateDraw();
			}
		}
	}

	return 1;
}

Uint32 UIConsole::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( flags == EE_BUTTON_WUMASK ) {
		if ( mCon.min - mCon.modif - 6 > 0 ) {
			mCon.modif += 6;
		} else {
			mCon.modif = mCon.min;
		}
	} else if ( flags == EE_BUTTON_WDMASK ) {
		if ( mCon.modif - 6 > 0 ) {
			mCon.modif -= 6;
		} else {
			mCon.modif = 0;
		}
	} else if ( flags & EE_BUTTON_LMASK ) {
		if ( mMouseDown ) {
			mMouseDown = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		}
	} else if ( ( flags & EE_BUTTON_RMASK ) ) {
		onCreateContextMenu( position, flags );
	}
	return UIWidget::onMouseUp( position, flags );
}

void UIConsole::onDocumentTextChanged( const DocumentContentChange& ) {
	resetCursor();

	invalidateDraw();

	sendCommonEvent( Event::OnBufferChange );
}

void UIConsole::onDocumentCursorChange( const TextPosition& ) {
	resetCursor();
	invalidateDraw();
}

void UIConsole::onDocumentSelectionChange( const TextRange& ) {
	onSelectionChange();
}

void UIConsole::onDocumentLineCountChange( const size_t&, const size_t& ) {
	invalidateDraw();
}

void UIConsole::onDocumentLineChanged( const Int64& ) {
	invalidateDraw();
}

void UIConsole::onDocumentUndoRedo( const TextDocument::UndoRedo& ) {
	onSelectionChange();
}

void UIConsole::onDocumentSaved( TextDocument* ) {}

void UIConsole::onDocumentMoved( TextDocument* ) {}

Drawable* UIConsole::findIcon( const std::string& name ) {
	UIIcon* icon = getUISceneNode()->findIcon( name );
	if ( icon )
		return icon->getSize( mMenuIconSize );
	return nullptr;
}

void UIConsole::copySelection() {
	std::string str;
	auto selNorm = mSelection.normalized();

	for ( Int64 i = selNorm.start().line(); i <= selNorm.end().line(); i++ ) {
		auto startCol = eemin( (Int64)mCmdLog[i].log.size(), selNorm.start().column() );
		auto endCol = eemin( (Int64)mCmdLog[i].log.size(), selNorm.end().column() );

		if ( i >= (Int64)mCmdLog.size() )
			continue;

		if ( i == selNorm.start().line() ) {
			str += mCmdLog[i]
					   .log
					   .substr( startCol, selNorm.end().line() == i
											  ? eemax( (Int64)0, endCol - startCol )
											  : (Int64)mCmdLog[i].log.size() - startCol )
					   .toUtf8();
			if ( endCol == (Int64)mCmdLog[i].log.size() )
				str += "\n";
		} else if ( i == selNorm.end().line() ) {
			str += mCmdLog[i].log.substr( 0, endCol ).toUtf8();
			if ( endCol == (Int64)mCmdLog[i].log.size() )
				str += "\n";
		} else {
			str += mCmdLog[i].log.toUtf8();
			str += "\n";
		}
	}

	getUISceneNode()->getWindow()->getClipboard()->setText( str );
}

UIMenuItem* UIConsole::menuAdd( UIPopUpMenu* menu, const String& translateString,
								const std::string& icon, const std::string& cmd ) {
	UIMenuItem* menuItem =
		menu->add( translateString, findIcon( icon ), mKeyBindings.getCommandKeybindString( cmd ) );
	menuItem->setId( cmd );
	return menuItem;
}

bool UIConsole::onCreateContextMenu( const Vector2i& position, const Uint32& flags ) {
	if ( mCurrentMenu )
		return false;

	UIPopUpMenu* menu = UIPopUpMenu::New();

	menuAdd( menu, i18n( "uiconsole_copy", "Copy" ), "copy", "copy" )
		->setEnabled( mSelection.hasSelection() );

	ContextMenuEvent event( this, menu, Event::OnCreateContextMenu, position, flags );
	sendEvent( &event );

	if ( menu->getCount() == 0 ) {
		menu->close();
		return false;
	}

	menu->setCloseOnHide( true );
	menu->addEventListener( Event::OnItemClicked, [this, menu]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const std::string& txt( item->getId() );
		if ( txt == "copy" )
			copySelection();
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
	menu->addEventListener( Event::OnMenuHide, [this]( const Event* ) {
		if ( !isClosing() )
			setFocus();
	} );
	menu->addEventListener( Event::OnClose, [this]( const Event* ) { mCurrentMenu = nullptr; } );
	return true;
}

void UIConsole::onSelectionChange() {
	invalidateDraw();
}

String UIConsole::getLastCommonSubStr( std::vector<String>& cmds ) {
	String lastCommon( mDoc.getCurrentLine().getTextWithoutNewLine() );
	String strTry( lastCommon );

	std::vector<String>::iterator ite;

	bool found = false;

	do {
		found = false;
		String strBeg( ( *cmds.begin() ) );

		if ( strTry.size() + 1 <= strBeg.size() ) {
			bool allEqual = true;
			strTry = String( strBeg.substr( 0, strTry.size() + 1 ) );

			for ( ite = ++cmds.begin(); ite != cmds.end(); ++ite ) {
				String& strCur = ( *ite );

				if ( !( strTry.size() <= strCur.size() &&
						strTry == strCur.substr( 0, strTry.size() ) ) ) {
					allEqual = false;
				}
			}

			if ( allEqual ) {
				lastCommon = strTry;

				found = true;
			}
		}
	} while ( found );

	return lastCommon;
}

void UIConsole::printCommandsStartingWith( const String& start ) {
	std::vector<String> cmds;

	for ( auto it = mCallbacks.begin(); it != mCallbacks.end(); ++it ) {
		if ( String::startsWith( it->first, start ) )
			cmds.push_back( it->first );
	}

	if ( cmds.size() > 1 ) {
		privPushText( "> " + mDoc.getCurrentLine().getTextWithoutNewLine() );

		for ( auto& cmd : cmds )
			privPushText( std::move( cmd ) );

		String newStr( getLastCommonSubStr( cmds ) );

		if ( newStr != mDoc.getCurrentLine().getTextWithoutNewLine() ) {
			mDoc.replaceCurrentLine( newStr );
			mDoc.moveToEndOfLine();
		}
	} else if ( cmds.size() ) {
		mDoc.replaceCurrentLine( cmds.front() );
		mDoc.moveToEndOfLine();
	}
}

void UIConsole::updateCacheSize() {
	Int32 maxLines = maxLinesOnScreen();
	if ( maxLines > (Int64)mTextCache.size() )
		mTextCache.resize( maxLines );
}

void UIConsole::onSizeChange() {
	updateCacheSize();
	return UIWidget::onSizeChange();
}

void UIConsole::onParentSizeChange( const Vector2f& sizeChange ) {
	updateQuakeMode();
	return UIWidget::onParentSizeChange( sizeChange );
}

void UIConsole::getFilesFrom( std::string txt, const Uint32& curPos ) {
	static char OSSlash = FileSystem::getOSSlash().at( 0 );
	size_t pos;

	if ( std::string::npos != ( pos = txt.find_last_of( OSSlash ) ) && pos <= curPos ) {
		size_t fpos = txt.find_first_of( OSSlash );

		std::string dir( txt.substr( fpos, pos - fpos + 1 ) );
		std::string file( txt.substr( pos + 1 ) );

		if ( FileSystem::isDirectory( dir ) ) {
			size_t count = 0, lasti = 0;
			std::vector<std::string> files = FileSystem::filesGetInPath( dir, true, true );
			String res;
			bool again = false;

			do {
				std::vector<std::string> foundFiles;
				res = "";
				count = 0;
				again = false;

				for ( size_t i = 0; i < files.size(); i++ ) {
					if ( !file.size() || String::startsWith( files[i], file ) ) {
						res += "\t" + files[i] + "\n";
						count++;
						lasti = i;
						foundFiles.push_back( files[i] );
					}
				}

				if ( count > 1 ) {
					bool allBigger = true;
					bool allStartsWith = true;

					do {
						allBigger = true;

						for ( size_t i = 0; i < foundFiles.size(); i++ ) {
							if ( foundFiles[i].size() < file.size() + 1 ) {
								allBigger = false;
								break;
							}
						}

						if ( allBigger ) {
							std::string tfile = foundFiles[0].substr( 0, file.size() + 1 );
							allStartsWith = true;

							for ( size_t i = 0; i < foundFiles.size(); i++ ) {
								if ( !String::startsWith( foundFiles[i], tfile ) ) {
									allStartsWith = false;
									break;
								}
							}

							if ( allStartsWith ) {
								file = tfile;
								again = true;
							}
						}
					} while ( allBigger && allStartsWith );
				}
			} while ( again );

			if ( count == 1 ) {
				std::string slash = "";

				if ( FileSystem::isDirectory( dir + files[lasti] ) ) {
					slash = FileSystem::getOSSlash();
				}

				mDoc.replaceCurrentLine( mDoc.getCurrentLine().getText().substr( 0, pos + 1 ) +
										 files[lasti] + slash );
			} else if ( count > 1 ) {
				privPushText( "Directory file list:" );
				pushText( res );
				mDoc.replaceCurrentLine( mDoc.getCurrentLine().getText().substr( 0, pos + 1 ) +
										 file );
			}
			mDoc.moveToEndOfLine();
			invalidateDraw();
		}
	}
}

void UIConsole::pushText( const String& str ) {
	if ( std::string::npos != str.find_first_of( '\n' ) ) {
		std::vector<String> Strings = String::split( String( str ) );

		for ( Uint32 i = 0; i < Strings.size(); i++ ) {
			privPushText( std::move( Strings[i] ) );
		}
	} else {
		privPushText( String( str ) );
	}
}

Float UIConsole::getLineHeight() const {
	return mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
}

bool UIConsole::getQuakeMode() const {
	return mQuakeMode;
}

void UIConsole::setQuakeMode( bool quakeMode ) {
	if ( mQuakeMode != quakeMode ) {
		mQuakeMode = quakeMode;
		updateQuakeMode();
	}
}

void UIConsole::updateQuakeMode() {
	if ( !mQuakeMode )
		return;
	setParent( mUISceneNode->getRoot() );
	Sizef ps( mUISceneNode->getRoot()->getPixelsSize() );
	setPixelsSize( { ps.getWidth(), eefloor( ps.getHeight() * mQuakeModeHeightPercent ) } );
	setPosition( { 0, 0 } );
}

void UIConsole::show() {
	if ( !mQuakeMode ) {
		setVisible( true );
		setEnabled( true );
		return;
	}
	if ( mHiding )
		return;

	setVisible( true );
	setEnabled( true );
	toFront();
	mFading = true;
	auto* spawn = Actions::Spawn::New(
		{ Actions::FadeIn::New( Seconds( .25f ) ),
		  Actions::Move::New( { 0, -getSize().getHeight() }, { 0, 0 }, Seconds( .25f ) ) } );
	runAction( Actions::Sequence::New( { spawn, Actions::Runnable::New( [this] {
											 setVisible( true );
											 setEnabled( true );
											 mFading = false;
											 setFocus();
										 } ) } ) );
}

void UIConsole::hide() {
	if ( !mQuakeMode ) {
		setVisible( false );
		setEnabled( false );
		return;
	}
	if ( mFading )
		return;

	mHiding = true;
	setVisible( true );
	setEnabled( true );
	auto* spawn = Actions::Spawn::New(
		{ Actions::FadeOut::New( Seconds( .25f ) ),
		  Actions::Move::New( { 0, 0 }, { 0, -getSize().getHeight() }, Seconds( .25f ) ) } );
	runAction( Actions::Sequence::New( { spawn, Actions::Runnable::New( [this] {
											 setVisible( false );
											 setEnabled( false );
											 mHiding = false;
										 } ) } ) );
}

void UIConsole::toggle() {
	if ( isVisible() ) {
		hide();
	} else {
		show();
	}
}

bool UIConsole::isActive() const {
	return isVisible() && !mHiding;
}

Float UIConsole::getQuakeModeHeightPercent() const {
	return mQuakeModeHeightPercent;
}

void UIConsole::setQuakeModeHeightPercent( const Float& quakeModeHeightPercent ) {
	mQuakeModeHeightPercent = quakeModeHeightPercent;
}

static std::vector<String> splitCommandParams( String str ) {
	std::vector<String> params = String::split( str, ' ' );
	std::vector<String> rparams;
	String tstr;

	for ( size_t i = 0; i < params.size(); i++ ) {
		String tparam = params[i];

		if ( !tparam.empty() ) {
			if ( '"' == tparam[0] ) {
				tstr += tparam;
			} else if ( '"' == tparam[tparam.size() - 1] ) {
				tstr += " " + tparam;

				rparams.push_back( String::trim( tstr, '"' ) );

				tstr = "";
			} else if ( !tstr.empty() ) {
				tstr += " " + tparam;
			} else {
				rparams.push_back( tparam );
			}
		}
	}

	if ( !tstr.empty() ) {
		rparams.push_back( String::trim( tstr, '"' ) );
	}

	return rparams;
}

void UIConsole::processLine() {
	String str( mDoc.getCurrentLine().getTextWithoutNewLine() );
	std::vector<String> params = splitCommandParams( str );

	mLastCommands.push_back( str );
	mLastLogPos = (int)mLastCommands.size();

	if ( mLastCommands.size() > 20 )
		mLastCommands.pop_front();

	if ( str.size() > 0 ) {
		privPushText( "> " + str );

		if ( mCallbacks.find( params[0] ) != mCallbacks.end() ) {
			mCallbacks[params[0]]( params );
		} else {
			privPushText( "Unknown Command: '" + params[0] + "'" );
		}
	}
	mDoc.replaceCurrentLine( "" );
	invalidateDraw();
}

}} // namespace EE::UI
