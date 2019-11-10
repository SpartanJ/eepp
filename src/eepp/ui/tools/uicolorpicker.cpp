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
	mHsv(0, 1, 1, 1),
	mRgb(Color::fromHsv(mHsv))
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
	#color_picker > .header > .current_color {
		background-color: white;
		foreground-color: red;
	}
	#color_picker > .header {
		margin-bottom: 4dp;
	}
	#color_picker > .header > .picker_icon {
		icon: color-picker-white;
		gravity: center;
	}
	#color_picker > .pickers > .color_picker_container > .color_picker {
		background-image: linear-gradient(white, black);
		scale-type: expand;
	}
	#color_picker > .pickers > .color_picker_container > .vertical_line {
		background-color: black;
		border-width: 1dp;
		border-color: white;
		margin-left: 128dp;
	}
	#color_picker > .pickers > .color_picker_container > .horizontal_line {
		background-color: black;
		border-width: 1dp;
		border-color: white;
		margin-top: 128dp;
	}
	#color_picker > .pickers > .separator,
	#color_picker > .header > .separator {
		margin: 8dp 0dp 8dp 0dp;
	}
	#color_picker > .pickers > .separator {
		background-color: #FFFFFF88;
	}
	#color_picker > .pickers > .hue_picker_container > .hue_picker {
		background-image: linear-gradient(red, black);
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
			<Widget class="current_color" layout_width="256dp" layout_height="match_parent" />
			<Widget class="separator" layout_width="1dp" layout_height="256dp" />
			<PushButton class="picker_icon" layout_width="0dp" layout_weight="1" layout_height="match_parent" />
		</LinearLayout>
		<LinearLayout class="pickers" orientation="horizontal" layout_width="match_parent" layout_height="256dp">
			<RelativeLayout class="color_picker_container" orientation="horizontal" layout_width="256dp" layout_height="256dp">
				<Image id="color_picker_rect" class="color_picker" layout_width="match_parent" layout_height="match_parent" />
				<Widget class="vertical_line" layout_width="1dp" layout_height="match_parent" />
				<Widget class="horizontal_line" layout_width="match_parent" layout_height="1dp" />
			</RelativeLayout>
			<Widget class="separator" layout_width="1dp" layout_height="256dp" />
			<RelativeLayout class="hue_picker_container" orientation="horizontal" layout_width="0dp" layout_weight="1" layout_height="256dp">
				<Image id="hue_picker" class="hue_picker" layout_width="match_parent" layout_height="match_parent" />
				<Widget class="hue_line" layout_width="match_parent" layout_height="1dp" />
			</RelativeLayout>
		</LinearLayout>
		<Widget class="separator" layout_width="match_parent" layout_height="1dp" />
		<LinearLayout class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="R" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" />
		</LinearLayout>
		<LinearLayout class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="G" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" />
		</LinearLayout>
		<LinearLayout class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="B" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" />
		</LinearLayout>
		<LinearLayout class="slider_container" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
			<TextView layout_width="16dp" layout_height="wrap_content" text="A" layout_gravity="center" />
			<Slider layout_width="0dp" layout_weight="1" layout_height="wrap_content" orientation="horizontal" layout_gravity="center" />
			<SpinBox layout_width="48dp" layout_height="wrap_content" />
		</LinearLayout>
		<LinearLayout class="footer" orientation="horizontal" ayout_width="match_parent" layout_height="wrap_content">
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
	mHueTexture = createHueTexture( mHuePicker->getPixelsSize() );
	mHuePicker->setDrawable( mHueTexture );
	updateColorPicker();
}

UIColorPicker::~UIColorPicker() {
	TextureFactory::instance()->remove( mHueTexture->getTextureId() );
	mColorPicker->setDrawable( NULL );
	eeSAFE_DELETE( mColorRectangle );
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
		hsva.hsv.h = 360.f * y / image.getHeight();
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
	mColorRectangle = RectangleDrawable::New();

	RectColors rectColors;
	rectColors.TopLeft = Color::fromHsv( Colorf( mHsv.hsv.h, 0, 1, 1 ) );
	rectColors.BottomLeft = Color::fromHsv( Colorf( mHsv.hsv.h, 0, 0, 1 ) );
	rectColors.BottomRight = Color::fromHsv( Colorf( mHsv.hsv.h, 1, 0, 1 ) );
	rectColors.TopRight = Color::fromHsv( Colorf( mHsv.hsv.h, 1, 1, 1 ) );
	mColorRectangle->setRectColors( rectColors );

	mColorPicker->setDrawable( mColorRectangle );
	eeSAFE_DELETE( oldDrawable );
}

}}}
