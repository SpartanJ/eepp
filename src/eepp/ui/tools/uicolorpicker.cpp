#include <eepp/graphics/drawablegroup.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/scene/actions/fade.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/uicolorpicker.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI { namespace Tools {

static CSS::StyleSheetParser sStyleSheetParser;
static UISceneNode* sLastSceneNode = NULL;

const char* COLOR_PICKER_STYLE = R"xml(
#color_picker {
	margin: 4dp;
	window-min-size: 320dp 478dp;
}
#color_picker > .header {
	margin-bottom: 4dp;
}
#color_picker > .header > .current_color {
}
#color_picker > .header > .picker_icon {
	icon: color-picker;
	gravity: center;
}
#color_picker > .pickers > .color_picker_container > .color_picker {
	background-color: white;
	scale-type: expand;
}
#color_picker > .pickers > .color_picker_container > .vertical_line {
	background-color: black;
	border-width: 1dp;
	border-color: white;
	border-type: outside;
}
#color_picker > .pickers > .color_picker_container > .horizontal_line {
	background-color: black;
	border-width: 1dp;
	border-color: white;
	border-type: outside;
}
#color_picker > .pickers > .separator,
#color_picker > .header > .separator {
	margin-left: 8dp;
	margin-right: 8dp;
}
#color_picker > .pickers > .separator {
	background-color: #FFFFFF88;
}
#color_picker > .pickers > .hue_picker_container > .hue_picker {
	background-color: white;
	scale-type: expand;
}
#color_picker > .pickers > .hue_picker_container > .hue_line {
	background-color: black;
	border-width: 1dp;
	border-color: white;
	border-type: outside;
}
#color_picker > .separator {
	margin-top: 8dp;
	margin-bottom: 4dp;
	background-color: #FFFFFF88;
}
#color_picker > .slider_container,
#color_picker > .footer {
	margin-top: 4dp;
}
#color_picker > .footer > PushButton {
	padding-left: 8dp;
	padding-right: 8dp;
	icon: ok;
}
)xml";

UIColorPicker* UIColorPicker::NewModal( Node* nodeCreator,
										const UIColorPicker::ColorPickedCb& colorPickedCb,
										const Uint8& modalAlpha, const Uint32& winFlags,
										const Sizef& winSize ) {
	UIColorPicker* colorPicker = NewWindow( colorPickedCb, winFlags, winSize, modalAlpha );
	UIWindow* pickerWin = colorPicker->getUIWindow();
	Sizef windowSize( pickerWin->getSceneNode()->getSize() );
	Sizef nodeSize( nodeCreator->getSize() );
	Vector2f nodePos( nodeCreator->getPosition() );
	nodeCreator->getParent()->nodeToWorld( nodePos );
	nodePos = PixelDensity::pxToDp( nodePos );

	if ( nodePos.y + nodeSize.getHeight() + winSize.getHeight() <= windowSize.getHeight() &&
		 nodePos.x + winSize.getWidth() <= windowSize.getWidth() ) {
		pickerWin->setPosition( nodePos.x, nodePos.y + nodeSize.getHeight() );
	} else if ( nodePos.y - winSize.getHeight() >= 0.f &&
				nodePos.x + winSize.getWidth() <= windowSize.getWidth() ) {
		pickerWin->setPosition( nodePos.x, nodePos.y - winSize.getHeight() );
	} else if ( nodePos.y + nodeSize.getHeight() / 2 - winSize.getHeight() / 2 >= 0 &&
				nodePos.y + nodeSize.getHeight() / 2 - winSize.getHeight() / 2 +
						winSize.getHeight() <=
					windowSize.getHeight() ) {
		if ( nodePos.x - winSize.getWidth() >= 0 ) {
			pickerWin->setPosition( nodePos.x - winSize.getWidth(), nodePos.y +
																		nodeSize.getHeight() / 2 -
																		winSize.getHeight() / 2 );
		} else if ( nodePos.x + nodeSize.getWidth() + winSize.getWidth() <=
					windowSize.getWidth() ) {
			pickerWin->setPosition( nodePos.x + nodeSize.getWidth(), nodePos.y +
																		 nodeSize.getHeight() / 2 -
																		 winSize.getHeight() / 2 );
		} else {
			pickerWin->center();
		}
	} else {
		pickerWin->center();
	}

	return colorPicker;
}

UIColorPicker* UIColorPicker::NewWindow( const ColorPickedCb& colorPickedCb, const Uint32& winFlags,
										 const Sizef& winSize, const Uint8& modalAlpha ) {
	Clock clock;
	UIWindow* tWin = UIWindow::New();
	tWin->setSizeWithDecoration( winSize )->setPosition( 0, 0 );
	UIWindow::StyleConfig windowStyleConfig = tWin->getStyleConfig();
	windowStyleConfig.WinFlags = winFlags;
	windowStyleConfig.MinWindowSize = winSize;
	tWin->setStyleConfig( windowStyleConfig );
	UIColorPicker* colorPicker = Tools::UIColorPicker::New( tWin, colorPickedCb, modalAlpha );
	tWin->show();
	Log::debug( "UIColorPicker created in: %.2fms", clock.getElapsedTime().asMilliseconds() );
	return colorPicker;
}

UIColorPicker* UIColorPicker::New( UIWindow* attachTo,
								   const UIColorPicker::ColorPickedCb& colorPickedCb,
								   const Uint8& modalAlpha ) {
	return eeNew( UIColorPicker, ( attachTo, colorPickedCb, modalAlpha ) );
}

UIColorPicker::UIColorPicker( UIWindow* attachTo, const UIColorPicker::ColorPickedCb& colorPickedCb,
							  const Uint8& modalAlpha ) :
	mUIWindow( attachTo ),
	mUIContainer( NULL ),
	mRoot( NULL ),
	mPickedCb( colorPickedCb ),
	mColorPicker( NULL ),
	mHuePicker( NULL ),
	mVerticalLine( NULL ),
	mHorizontalLine( NULL ),
	mHueLine( NULL ),
	mCurrentColor( NULL ),
	mRedContainer( NULL ),
	mGreenContainer( NULL ),
	mBlueContainer( NULL ),
	mAlphaContainer( NULL ),
	mFooter( NULL ),
	mHsv( 0, 1, 1, 1 ),
	mRgb( Color::fromHsv( mHsv ) ),
	mHexColor( mRgb.toHexString( false ) ),
	mModalAlpha( modalAlpha ),
	mDefModalAlpha( modalAlpha ),
	mUpdating( false ) {
	if ( NULL == mUIWindow ) {
		mUIContainer = SceneManager::instance()->getUISceneNode();
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	std::string layout = R"xml(
	<LinearLayout id="color_picker" class="container" orientation="vertical" layout_width="match_parent" layout_height="wrap_content">
		<LinearLayout class="header" orientation="horizontal" layout_width="match_parent" layout_height="28dp">
			<Widget id="current_color" class="current_color" layout_width="256dp" layout_height="match_parent" />
			<Widget class="separator" layout_width="1dp" layout_height="256dp" />
			<PushButton id="picker_icon" class="picker_icon" layout_width="0dp" layout_weight="1" layout_height="match_parent" />
		</LinearLayout>
		<LinearLayout class="pickers" orientation="horizontal" layout_width="match_parent" layout_height="256dp">
			<RelativeLayout class="color_picker_container" orientation="horizontal" layout_width="256dp" layout_height="256dp">
				<Image id="color_picker_rect" class="color_picker" layout_width="match_parent" layout_height="match_parent" />
				<Widget id="vertical_line" enabled="false" class="vertical_line" layout_width="1dp" layout_height="match_parent" />
				<Widget id="horizontal_line" enabled="false" class="horizontal_line" layout_width="match_parent" layout_height="1dp" />
			</RelativeLayout>
			<Widget class="separator" layout_width="1dp" layout_height="256dp" />
			<RelativeLayout class="hue_picker_container" orientation="horizontal" layout_width="0dp" layout_weight="1" layout_height="256dp">
				<Image id="hue_picker" class="hue_picker" layout_width="match_parent" layout_height="match_parent" />
				<Widget id="hue_line" enabled="false" class="hue_line" layout_width="match_parent" layout_height="1dp" />
			</RelativeLayout>
		</LinearLayout>
		<Widget class="separator" layout_width="match_parent" layout_height="1dp" />
		<LinearLayout id="red_container" class="slider_container" orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="R" layout_gravity="center" />
			<HSlider layout_width="0dp" layout_weight="1" layout_height="wrap_content" layout_gravity="center" minValue="0" maxValue="255" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="green_container" class="slider_container" orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="G" layout_gravity="center" />
			<HSlider layout_width="0dp" layout_weight="1" layout_height="wrap_content" layout_gravity="center" minValue="0" maxValue="255" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="blue_container" class="slider_container" orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="B" layout_gravity="center" />
			<HSlider layout_width="0dp" layout_weight="1" layout_height="wrap_content" layout_gravity="center" minValue="0" maxValue="255" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="alpha_container" class="slider_container" orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="A" layout_gravity="center" />
			<HSlider layout_width="0dp" layout_weight="1" layout_height="wrap_content" layout_gravity="center" minValue="0" maxValue="255" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="footer" class="footer" orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
			<Widget layout_width="0dp" layout_weight="1" layout_height="match_parent" />
			<TextView layout_width="wrap_content" layout_height="wrap_content" text="#" layout_gravity="center" />
			<TextInput layout_width="120dp" layout_height="wrap_content" />
			<PushButton layout_width="wrap_content" layout_height="wrap_content" text="OK" />
		</LinearLayout>
	</LinearLayout>
	)xml";

	if ( NULL != mUIContainer->getSceneNode() && mUIContainer->getSceneNode()->isUISceneNode() ) {
		Clock clock;
		UISceneNode* uiSceneNode = mUIContainer->getSceneNode()->asType<UISceneNode>();

		if ( !sStyleSheetParser.isLoaded() ) {
			sStyleSheetParser.loadFromString( std::string( COLOR_PICKER_STYLE ) );
		}

		if ( sLastSceneNode != uiSceneNode ) {
			uiSceneNode->combineStyleSheet( sStyleSheetParser.getStyleSheet() );
			sLastSceneNode = uiSceneNode;
		}

		mRoot = uiSceneNode->loadLayoutFromString( layout, mUIContainer );

		Log::debug( "UIColorPicker loadLayoutFromString time: %.2f",
					clock.getElapsedTime().asMilliseconds() );
	}

	if ( NULL != mUIWindow ) {
		mUIWindow->setTitle( "Color Picker" );
		mUIWindow->addEventListener( Event::OnWindowClose,
									 cb::Make1( this, &UIColorPicker::windowClose ) );
	} else {
		mUIContainer->addEventListener( Event::OnClose,
										cb::Make1( this, &UIColorPicker::windowClose ) );
	}

	if ( NULL != mUIWindow && mUIWindow->isModal() ) {
		if ( mModalAlpha != 0.f ) {
			mUIWindow->getModalWidget()->setBackgroundColor( Color( 0, 0, 0, mModalAlpha ) );

			if ( mRoot->getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
				mUIWindow->getModalWidget()->runAction( Actions::Fade::New(
					0.f, mModalAlpha,
					mRoot->getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ) );
			}
		}
		mUIWindow->getModalWidget()->addEventListener( Event::MouseClick,
													   [&]( const Event* ) { closePicker(); } );
		mUIWindow->addEventListener( Event::KeyDown, [&]( const Event* event ) {
			const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
			onKeyDown( *keyEvent );
		} );
	}

	mRoot->bind( "color_picker_rect", mColorPicker );
	mRoot->bind( "hue_picker", mHuePicker );
	mRoot->bind( "vertical_line", mVerticalLine );
	mRoot->bind( "horizontal_line", mHorizontalLine );
	mRoot->bind( "hue_line", mHueLine );
	mRoot->bind( "current_color", mCurrentColor );
	mRoot->bind( "red_container", mRedContainer );
	mRoot->bind( "green_container", mGreenContainer );
	mRoot->bind( "blue_container", mBlueContainer );
	mRoot->bind( "alpha_container", mAlphaContainer );
	mRoot->bind( "footer", mFooter );

	mRoot->addEventListener( Event::OnSizeChange, [&]( const Event* ) { updateAll(); } );

	mRoot->addEventListener( Event::OnLayoutUpdate, [&]( const Event* ) {
		if ( mHuePicker->getDrawable() == nullptr ) {
			mHuePicker->setDrawable( createHueTexture( mHuePicker->getPixelsSize() ), true );
			mCurrentColor->setBackgroundDrawable( createGridTexture(), true );
			mCurrentColor->setBackgroundRepeat( "repeat" );
			updateAll();
		}
	} );

	mRoot->setFocus();

	registerEvents();
}

void UIColorPicker::setColor( const Color& color ) {
	if ( color != mRgb ) {
		mRgb = color;
		mHsv = color.toHsv();
		mHexColor = mRgb.toHexString( false );
		updateAll();
	}
}

const Color& UIColorPicker::getColor() const {
	return mRgb;
}

void UIColorPicker::setHsvColor( const Colorf& color ) {
	if ( color != mHsv ) {
		mHsv = color;
		mRgb = Color::fromHsv( mHsv );
		mHexColor = mRgb.toHexString( false );
		updateAll();
	}
}

const Colorf& UIColorPicker::getHsvColor() const {
	return mHsv;
}

UIWindow* UIColorPicker::getUIWindow() const {
	return mUIWindow;
}

void UIColorPicker::closePicker() {
	if ( mModalAlpha != 0.f &&
		 mRoot->getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() )
		mUIWindow->getModalWidget()->runAction( Actions::FadeOut::New(
			mRoot->getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ) );
	mUIWindow->closeWindow();
}

void UIColorPicker::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_ESCAPE ) {
		closePicker();
	}
}

Uint8 UIColorPicker::getModalAlpha() const {
	return mModalAlpha;
}

void UIColorPicker::setModalAlpha( const Uint8& modalAlpha ) {
	if ( modalAlpha != mModalAlpha ) {
		mModalAlpha = modalAlpha;

		if ( NULL != mUIWindow && mUIWindow->isModal() ) {
			mUIWindow->getModalWidget()->setBackgroundColor( Color( 0, 0, 0, mModalAlpha ) );
		}
	}
}

void UIColorPicker::windowClose( const Event* ) {
	eeDelete( this );
}

Texture* UIColorPicker::createHueTexture( const Sizef& size ) {
	Image image( 1, (Uint32)size.getHeight(), 3 );

	for ( Uint32 y = 0; y < image.getHeight(); y++ ) {
		Colorf hsva;
		hsva.hsv.h = 360.f - 360.f * y / image.getHeight();
		hsva.hsv.s = 1.f;
		hsva.hsv.v = 1.f;
		image.setPixel( 0, y, Color::fromHsv( hsva ) );
	}

	TextureFactory* TF = TextureFactory::instance();
	Uint32 texId = TF->loadFromPixels( image.getPixelsPtr(), image.getWidth(), image.getHeight(),
									   image.getChannels() );

	return TF->getTexture( texId );
}

Texture* UIColorPicker::createGridTexture() {
	Sizef size( PixelDensity::dpToPx( Sizef( 26, 24 ) ) );
	Image image( size.getWidth(), size.getHeight(), 3, Color( 128, 128, 128, 255 ) );
	Color highlightColor( 204, 204, 204, 255 );
	int hWidth = size.getWidth() / 2;
	int hHeight = size.getHeight() / 2;
	for ( int y = 0; y < hHeight; y++ ) {
		for ( int x = 0; x < hWidth; x++ ) {
			image.setPixel( x, y, highlightColor );
			image.setPixel( hWidth + x, hHeight + y, highlightColor );
		}
	}

	TextureFactory* TF = TextureFactory::instance();
	Uint32 texId = TF->loadFromPixels( image.getPixelsPtr(), image.getWidth(), image.getHeight(),
									   image.getChannels() );

	return TF->getTexture( texId );
}

void UIColorPicker::updateColorPicker() {
	DrawableGroup* colorRectangle = DrawableGroup::New();

	RectangleDrawable* rectDrawable = RectangleDrawable::New();

	RectColors rectColors;
	rectDrawable->setSize( mColorPicker->getPixelsSize() );
	rectColors.TopLeft = Color::fromHsv( Colorf( mHsv.hsv.h, 0, 1, 1 ) );
	rectColors.BottomLeft = Color::fromHsv( Colorf( mHsv.hsv.h, 0, 1, 1 ) );
	rectColors.BottomRight = Color::fromHsv( Colorf( mHsv.hsv.h, 1, 1, 1 ) );
	rectColors.TopRight = Color::fromHsv( Colorf( mHsv.hsv.h, 1, 1, 1 ) );
	rectDrawable->setRectColors( rectColors );
	colorRectangle->addDrawable( rectDrawable );

	rectDrawable = RectangleDrawable::New();
	rectDrawable->setSize( mColorPicker->getPixelsSize() );
	rectColors.TopLeft = Color::Transparent;
	rectColors.BottomLeft = Color::Black;
	rectColors.BottomRight = Color::Black;
	rectColors.TopRight = Color::Transparent;
	rectDrawable->setRectColors( rectColors );
	colorRectangle->addDrawable( rectDrawable );

	mColorPicker->setDrawable( colorRectangle, true );
}

void UIColorPicker::updateGuideLines() {
	mVerticalLine->setLayoutMargin(
		Rectf( mColorPicker->getSize().getWidth() * mHsv.hsv.s, 0.f, 0.f, 0.f ) );
	mHorizontalLine->setLayoutMargin( Rectf(
		0.f, mColorPicker->getSize().getHeight() - mColorPicker->getSize().getHeight() * mHsv.hsv.v,
		0.f, 0.f ) );
	mHueLine->setLayoutMargin( Rectf( 0.f,
									  mHuePicker->getSize().getHeight() -
										  mHsv.hsv.h / 360.f * mHuePicker->getSize().getHeight(),
									  0.f, 0.f ) );
	mCurrentColor->setForegroundColor( Color::fromHsv( mHsv ) );
}

void UIColorPicker::updateChannelWidgets() {
	mRedContainer->findByTag<UISlider>( "slider" )->setValue( mRgb.r );
	mGreenContainer->findByTag<UISlider>( "slider" )->setValue( mRgb.g );
	mBlueContainer->findByTag<UISlider>( "slider" )->setValue( mRgb.b );
	mAlphaContainer->findByTag<UISlider>( "slider" )->setValue( mRgb.a );
	mRedContainer->findByTag<UISpinBox>( "spinbox" )->setValue( mRgb.r );
	mGreenContainer->findByTag<UISpinBox>( "spinbox" )->setValue( mRgb.g );
	mBlueContainer->findByTag<UISpinBox>( "spinbox" )->setValue( mRgb.b );
	mAlphaContainer->findByTag<UISpinBox>( "spinbox" )->setValue( mRgb.a );
	mFooter->findByTag<UITextInput>( "textinput" )->setText( mHexColor );
}

void UIColorPicker::updateAll() {
	mUpdating = true;
	updateColorPicker();
	updateGuideLines();
	updateChannelWidgets();
	mUpdating = false;
}

void UIColorPicker::registerEvents() {
	mColorPicker->addEventListener( Event::MouseDown, [&]( const Event* event ) {
		onColorPickerEvent( reinterpret_cast<const MouseEvent*>( event ) );
	} );

	mHuePicker->addEventListener( Event::MouseDown, [&]( const Event* event ) {
		onHuePickerEvent( reinterpret_cast<const MouseEvent*>( event ) );
	} );

	UISlider* redSlider = mRedContainer->findByTag<UISlider>( "slider" );

	redSlider->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( event->getNode()->asType<UISlider>()->getValue(), mRgb.g, mRgb.b, mRgb.a ) );
	} );

	UISlider* greenSlider = mGreenContainer->findByTag<UISlider>( "slider" );

	greenSlider->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( mRgb.r, event->getNode()->asType<UISlider>()->getValue(), mRgb.b, mRgb.a ) );
	} );

	UISlider* blueSlider = mBlueContainer->findByTag<UISlider>( "slider" );

	blueSlider->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( mRgb.r, mRgb.g, event->getNode()->asType<UISlider>()->getValue(), mRgb.a ) );
	} );

	UISlider* alphaSlider = mAlphaContainer->findByTag<UISlider>( "slider" );

	alphaSlider->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( mRgb.r, mRgb.g, mRgb.b, event->getNode()->asType<UISlider>()->getValue() ) );
	} );

	UISpinBox* redSpinBox = mRedContainer->findByTag<UISpinBox>( "spinbox" );

	redSpinBox->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( event->getNode()->asType<UISpinBox>()->getValue(), mRgb.g, mRgb.b, mRgb.a ) );
	} );

	UISpinBox* greenSpinBox = mGreenContainer->findByTag<UISpinBox>( "spinbox" );

	greenSpinBox->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( mRgb.r, event->getNode()->asType<UISpinBox>()->getValue(), mRgb.b, mRgb.a ) );
	} );

	UISpinBox* blueSpinBox = mBlueContainer->findByTag<UISpinBox>( "spinbox" );

	blueSpinBox->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( mRgb.r, mRgb.g, event->getNode()->asType<UISpinBox>()->getValue(), mRgb.a ) );
	} );

	UISpinBox* alphaSpinBox = mAlphaContainer->findByTag<UISpinBox>( "spinbox" );

	alphaSpinBox->addEventListener( Event::OnValueChange, [&]( const Event* event ) {
		if ( mUpdating )
			return;
		setColor(
			Color( mRgb.r, mRgb.g, mRgb.b, event->getNode()->asType<UISpinBox>()->getValue() ) );
	} );

	mFooter->findByTag<UIPushButton>( "pushbutton" )
		->addEventListener( Event::MouseClick, [&]( const Event* ) {
			if ( mPickedCb )
				mPickedCb( mRgb );

			if ( NULL != mUIWindow )
				mUIWindow->closeWindow();
		} );

	mFooter->findByTag<UITextInput>( "textinput" )
		->addEventListener( Event::OnPressEnter, [&]( const Event* event ) {
			if ( mUpdating )
				return;
			UITextInput* textInput = event->getNode()->asType<UITextInput>();
			std::string buffer( textInput->getText().toUtf8() );
			std::string colorString = "#" + buffer;

			if ( Color::validHexColorString( colorString ) ) {
				setColor( Color::fromString( colorString ) );
			} else {
				setColor( mRgb );
			}
		} );

	mRoot->find<UIPushButton>( "picker_icon" )
		->addEventListener( Event::MouseClick, [&]( const Event* ) {
			UIWidget* coverWidget = UIWidget::New();
			setModalAlpha( 0 );
			coverWidget->setParent( mRoot->getSceneNode() )
				->setPosition( 0, 0 )
				->setSize( mRoot->getSceneNode()->getSize() );
			coverWidget->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT |
									 UI_ANCHOR_BOTTOM );
			coverWidget->addEventListener( Event::MouseMove, [&]( const Event* event ) {
				Vector2i position = reinterpret_cast<const MouseEvent*>( event )->getPosition();
				setColor( GLi->readPixel(
					position.x,
					event->getNode()->getSceneNode()->getWindow()->getHeight() - position.y ) );
			} );
			coverWidget->addEventListener( Event::MouseClick, [&]( const Event* event ) {
				Vector2i position = reinterpret_cast<const MouseEvent*>( event )->getPosition();
				setColor( GLi->readPixel(
					position.x,
					event->getNode()->getSceneNode()->getWindow()->getHeight() - position.y ) );
				event->getNode()->close();
				setModalAlpha( mDefModalAlpha );
			} );
		} );
}

void UIColorPicker::onColorPickerEvent( const MouseEvent* mouseEvent ) {
	Vector2f nodePos( mouseEvent->getPosition().x, mouseEvent->getPosition().y );
	mouseEvent->getNode()->worldToNode( nodePos );
	nodePos = PixelDensity::dpToPx( nodePos );
	Float s = nodePos.x / mouseEvent->getNode()->asType<UIWidget>()->getPixelsSize().getWidth();
	Float v =
		1.f - nodePos.y / mouseEvent->getNode()->asType<UIWidget>()->getPixelsSize().getHeight();
	setHsvColor( Colorf( mHsv.hsv.h, s, v, mHsv.hsv.a ) );
}

void UIColorPicker::onHuePickerEvent( const MouseEvent* mouseEvent ) {
	Vector2f nodePos( mouseEvent->getPosition().x, mouseEvent->getPosition().y );
	mouseEvent->getNode()->worldToNode( nodePos );
	nodePos = PixelDensity::dpToPx( nodePos );
	Float h = 360.f - 360.f * nodePos.y /
						  mouseEvent->getNode()->asType<UIWidget>()->getPixelsSize().getHeight();
	setHsvColor( Colorf( h, mHsv.hsv.s, mHsv.hsv.v, mHsv.hsv.a ) );
}

}}} // namespace EE::UI::Tools
