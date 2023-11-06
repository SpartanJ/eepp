#include "uitextureviewer.hpp"
#include <eepp/graphics/textureloader.hpp>
#include <eepp/ui/uigridlayout.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI { namespace Tools {

UITextureViewer* UITextureViewer::New() {
	return eeNew( UITextureViewer, () );
}

UITextureViewer::~UITextureViewer() {
	TextureLoader::popLoadedCallback( mLoaderCb );
	for ( const auto& cb : mCbs ) {
		if ( !cb.first->popResourceChangeCallback( cb.second ) ) {
			eePRINTL( "UITextureViewer::~UITextureViewer popResourceChangeCallback failed" );
			eeASSERT( false );
		}
	}
}

UITextureViewer::UITextureViewer() : UIRelativeLayout( "textureviewer" ) {
	init();
}

void UITextureViewer::setImage( Drawable* drawable ) {
	UIImage* imageView = mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE );
	if ( imageView == nullptr )
		return;
	mImageLayout->setEnabled( true )->setVisible( true );
	imageView->setDrawable( drawable );
}

void UITextureViewer::init() {
	mUISceneNode->loadLayoutFromString(
		R"xml(
		<ScrollView layout_width="match_parent" layout_height="match_parent" touchdrag="true">
			<GridLayout columnMode="size" rowMode="size" columnWidth="200dp" rowHeight="200dp" layout_width="match_parent" layout_height="wrap_content" clip="false" />
		</ScrollView>
		<RelativeLayout class="image_container" lw="mp" lh="mp" visible="false" enabled="false" background-color="black">
			<Image lw="mp" lh="mp" scaleType="fit_inside" gravity="center" enabled="false" lg="center" />
			<TextView id="image_close" lw="wc" lh="wc" text="&#xeb99;" lg="top|right" enabled="false" />
		</RelativeLayout>
	)xml",
		this );

	mGridLayout = findByType<UIGridLayout>( UI_TYPE_GRID_LAYOUT );
	mImageLayout = findByClass<UIRelativeLayout>( "image_container" );
	auto hideImg = [this]( const Event* ) {
		mImageLayout->setEnabled( false )->setVisible( false );
	};
	mImageLayout->on( Event::MouseClick, hideImg );
	mImageLayout->on( Event::KeyDown, [hideImg]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
			hideImg( event );
	} );

	std::vector<Texture*> textures = TextureFactory::instance()->getTextures();
	for ( Texture* texture : textures )
		insertTexture( texture );

	mLoaderCb = TextureLoader::pushLoadedCallback(
		[this]( Uint32, Texture* tex ) { insertTexture( tex ); } );
}

static std::string getTextureDescription( Texture* tex ) {
	return String::format( "Name: %s\nSize: %dx%d", tex->getName().c_str(), tex->getWidth(),
						   tex->getHeight() );
}

void UITextureViewer::insertTexture( Texture* tex ) {
	UIImage* img = UIImage::New();
	std::string uid( String::format( "%llu", reinterpret_cast<Uint64>( tex ) ) );
	img->setDrawable( tex )
		->setScaleType( UIScaleType::FitInside )
		->setClasses( { "texture-preview", uid } )
		->setTooltipText( getTextureDescription( tex ) )
		->setGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER )
		->setEnabled( true )
		->setParent( mGridLayout )
		->onClick( [this, tex]( auto ) { setImage( tex ); } );
	Uint32 cb = tex->pushResourceChangeCallback(
		[this, uid]( Uint32, DrawableResource::Event event, DrawableResource* res ) {
			if ( event == DrawableResource::Event::Unload ) {
				auto found = mGridLayout->findByClass( uid );
				if ( found && mCbs.erase( static_cast<Texture*>( res ) ) )
					found->close();
			} else if ( event == DrawableResource::Change ) {
				getTextureDescription( static_cast<Texture*>( res ) );
			}
		} );
	mCbs.insert( { tex, cb } );
}

}}} // namespace EE::UI::Tools
