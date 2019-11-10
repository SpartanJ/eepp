#include <eepp/ui/tools/uicolorpicker.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/drawablegroup.hpp>

namespace EE { namespace UI { namespace Tools {

UIColorPicker* UIColorPicker::New( UIWindow* attachTo, const UIColorPicker::ColorPickedCb& colorPickedCb, const UIColorPicker::ColorPickerCloseCb& closeCb ) {
	return eeNew( UIColorPicker, ( attachTo, colorPickedCb, closeCb ) );
}

UIColorPicker::UIColorPicker( UIWindow* attachTo, const UIColorPicker::ColorPickedCb& colorPickedCb, const UIColorPicker::ColorPickerCloseCb& closeCb ) :
	mUIWindow( attachTo ),
	mPickedCb( colorPickedCb ),
	mCloseCb( closeCb ),
	mHueTexture( NULL ),
	mColorRectangle( NULL ),
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
	mHsv(0, 1, 1, 1),
	mRgb(Color::fromHsv(mHsv)),
	mUpdating(false)
{
	if ( NULL == mUIWindow ) {
		mUIContainer = SceneManager::instance()->getUISceneNode();
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	std::string layout = R"xml(
	<style>
	#color_picker {
		margin: 4dp;
		min-window-size: 320dp, 478dp;
	}
	#color_picker > .header {
		margin-bottom: 4dp;
	}
	#color_picker > .header > .picker_icon {
		icon: color-picker-white;
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
	}
	#color_picker > .pickers > .color_picker_container > .horizontal_line {
		background-color: black;
		border-width: 1dp;
		border-color: white;
	}
	#color_picker > .pickers > .separator,
	#color_picker > .header > .separator {
		margin: 8dp 0dp 8dp 0dp;
	}
	#color_picker > .pickers > .separator {
		background-color: #FFFFFF88;
	}
	#color_picker > .pickers > .hue_picker_container > .hue_picker {
		scale-type: expand;
	}
	#color_picker > .pickers > .hue_picker_container > .hue_line {
		background-color: black;
		border-width: 1dp;
		border-color: white;
	}
	#color_picker > .separator {
		margin: 0dp 8dp 0dp 4dp;
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
	</style>
	<LinearLayout id="color_picker" class="container" orientation="vertical" layout_width="match_parent" layout_height="wrap_content">
		<LinearLayout class="header" orientation="horizontal" layout_width="match_parent" layout_height="28dp">
			<Widget id="current_color" class="current_color" layout_width="256dp" layout_height="match_parent" />
			<Widget class="separator" layout_width="1dp" layout_height="256dp" />
			<PushButton class="picker_icon" layout_width="0dp" layout_weight="1" layout_height="match_parent" />
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
		<LinearLayout id="red_container" class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="R" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="green_container" class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="G" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" minValue="0" maxValue="255" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="blue_container" class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="B" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" minValue="0" maxValue="255" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="alpha_container" class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="A" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" minValue="0" maxValue="255" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" minValue="0" maxValue="255" />
		</LinearLayout>
		<LinearLayout id="footer" class="footer" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="wrap_content" layout_height="wrap_content" text="HTML Notation #" layout_gravity="center" />
			<TextInput layout_width="120dp" layout_height="wrap_content" />
			<Widget layout_width="0dp" layout_weight="1" layout_height="match_parent" />
			<PushButton layout_width="wrap_content" layout_height="wrap_content" text="OK" />
		</LinearLayout>
	</LinearLayout>
	)xml";

	UIWidget * root = NULL;
	if ( NULL != mUIContainer->getSceneNode() && mUIContainer->getSceneNode()->isUISceneNode() )
		root = mUIContainer->getSceneNode()->asType<UISceneNode>()->loadLayoutFromString( layout, mUIContainer );

	if ( NULL != mUIWindow ) {
		mUIWindow->setTitle( "Color Picker" );
		mUIWindow->addEventListener( Event::OnWindowClose, cb::Make1( this, &UIColorPicker::windowClose ) );
	} else {
		mUIContainer->addEventListener( Event::OnClose, cb::Make1( this, &UIColorPicker::windowClose ) );
	}

	root->bind( "color_picker_rect", mColorPicker );
	root->bind( "hue_picker", mHuePicker );
	root->bind( "vertical_line", mVerticalLine );
	root->bind( "horizontal_line", mHorizontalLine );
	root->bind( "hue_line", mHueLine );
	root->bind( "current_color", mCurrentColor );
	root->bind( "red_container", mRedContainer );
	root->bind( "green_container", mGreenContainer );
	root->bind( "blue_container", mBlueContainer );
	root->bind( "alpha_container", mAlphaContainer );
	root->bind( "footer", mFooter );

	root->addEventListener( Event::OnSizeChange, [&] ( const Event * event ) {
		updateAll();
	} );
	mHueTexture = createHueTexture( mHuePicker->getPixelsSize() );
	mHuePicker->setDrawable( mHueTexture );

	updateAll();

	mColorPicker->addEventListener( Event::MouseDown, [&]( const Event * event ) {
		onColorPickerEvent( reinterpret_cast<const MouseEvent*>( event ) );
	} );

	mHuePicker->addEventListener( Event::MouseDown, [&]( const Event * event ) {
		onHuePickerEvent( reinterpret_cast<const MouseEvent*>( event ) );
	} );

	registerEvents();
}

UIColorPicker::~UIColorPicker() {
	TextureFactory::instance()->remove( mHueTexture->getTextureId() );
	mColorPicker->setDrawable( NULL );
	eeSAFE_DELETE( mColorRectangle );
}

void UIColorPicker::setColor( const Color& color ) {
	mRgb = color;
	mHsv = color.toHsv();
	updateAll();
}

const Color& UIColorPicker::getColor() const {
	return mRgb;
}

void UIColorPicker::setHsvColor(const Colorf& color) {
	mHsv = color;
	mRgb = Color::fromHsv( mHsv );
	updateAll();
}

const Colorf& UIColorPicker::getHsvColor() const {
	return mHsv;
}

void UIColorPicker::windowClose( const Event * ) {
	if ( mCloseCb )
		mCloseCb();

	eeDelete( this );
}

Texture * UIColorPicker::createHueTexture( const Sizef& size ) {
	Image image( 1, (Uint32)size.getHeight(), 3 );

	for ( Uint32 y = 0; y < image.getHeight(); y++ ) {
		Colorf hsva;
		hsva.hsv.h = 360.f - 360.f * y / image.getHeight();
		hsva.hsv.s = 1.f;
		hsva.hsv.v = 1.f;
		image.setPixel( 0, y, Color::fromHsv( hsva ) );
	}

	TextureFactory * TF = TextureFactory::instance();
	Uint32 texId = TF->loadFromPixels( image.getPixelsPtr(), image.getWidth(), image.getHeight(),
									   image.getChannels(), false, Texture::ClampMode::ClampRepeat );

	return TF->getTexture( texId );
}

void UIColorPicker::updateColorPicker() {
	Drawable * oldDrawable = mColorRectangle;
	mColorRectangle = DrawableGroup::New();

	RectangleDrawable * rectDrawable = RectangleDrawable::New();

	RectColors rectColors;
	rectDrawable->setSize( mColorPicker->getPixelsSize() );
	rectColors.TopLeft = Color::fromHsv( Colorf( mHsv.hsv.h, 0, 1, 1 ) );
	rectColors.BottomLeft = Color::fromHsv( Colorf( mHsv.hsv.h, 0, 1, 1 ) );
	rectColors.BottomRight = Color::fromHsv( Colorf( mHsv.hsv.h, 1, 1, 1 ) );
	rectColors.TopRight = Color::fromHsv( Colorf( mHsv.hsv.h, 1, 1, 1 ) );
	rectDrawable->setRectColors( rectColors );
	mColorRectangle->addDrawable( rectDrawable );

	rectDrawable = RectangleDrawable::New();
	rectDrawable->setSize( mColorPicker->getPixelsSize() );
	rectColors.TopLeft = Color::Transparent;
	rectColors.BottomLeft = Color::Black;
	rectColors.BottomRight = Color::Black;
	rectColors.TopRight = Color::Transparent;
	rectDrawable->setRectColors( rectColors );
	mColorRectangle->addDrawable( rectDrawable );

	mColorPicker->setDrawable( mColorRectangle );
	eeSAFE_DELETE( oldDrawable );
}

void UIColorPicker::updateGuideLines() {
	mVerticalLine->setLayoutMargin( Rect( mColorPicker->getSize().getWidth() * mHsv.hsv.s, 0.f, 0.f, 0.f ) );
	mHorizontalLine->setLayoutMargin( Rect( 0.f, mColorPicker->getSize().getHeight() - mColorPicker->getSize().getHeight() * mHsv.hsv.v, 0.f, 0.f ) );
	mHueLine->setLayoutMargin( Rect( 0.f, mHuePicker->getSize().getHeight() - mHsv.hsv.h / 360.f * mHuePicker->getSize().getHeight(), 0.f, 0.f ) );
	mCurrentColor->setBackgroundColor( Color::fromHsv( mHsv ) );

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
	mFooter->findByTag<UITextInput>( "textinput" )->setText( mRgb.toHexString() );
}

void UIColorPicker::updateAll() {
	mUpdating = true;
	updateColorPicker();
	updateGuideLines();
	updateChannelWidgets();
	mUpdating = false;
}

void UIColorPicker::registerEvents() {

}

void UIColorPicker::unregisterEvents() {
	for ( auto& event : mEventsIds ) {
		event.first->removeEventListener( event.second );
	}
}

void UIColorPicker::onColorPickerEvent( const MouseEvent* mouseEvent ) {
	Vector2f controlPos( mouseEvent->getPosition().x, mouseEvent->getPosition().y );
	mouseEvent->getNode()->worldToNode( controlPos );
	controlPos = PixelDensity::dpToPx( controlPos );
	Float s = controlPos.x / mouseEvent->getNode()->asType<UIWidget>()->getPixelsSize().getWidth();
	Float v = 1.f - controlPos.y / mouseEvent->getNode()->asType<UIWidget>()->getPixelsSize().getHeight();
	setHsvColor( Colorf( mHsv.hsv.h, s, v, mHsv.hsv.a ) );
}

void UIColorPicker::onHuePickerEvent( const MouseEvent* mouseEvent ) {
	Vector2f controlPos( mouseEvent->getPosition().x, mouseEvent->getPosition().y );
	mouseEvent->getNode()->worldToNode( controlPos );
	controlPos = PixelDensity::dpToPx( controlPos );
	Float h = 360.f - 360.f * controlPos.y / mouseEvent->getNode()->asType<UIWidget>()->getPixelsSize().getHeight();
	setHsvColor( Colorf( h, mHsv.hsv.s, mHsv.hsv.v, mHsv.hsv.a ) );
}

}}}
