#include "uitextureviewer.hpp"
#include <eepp/graphics/textureloader.hpp>
#include <eepp/ui/uigridlayout.hpp>
#include <eepp/ui/uiimage.hpp>
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

UITextureViewer::UITextureViewer() : UILinearLayout( "textureviewer", UIOrientation::Vertical ) {
	init();
}

void UITextureViewer::init() {
	mUISceneNode->loadLayoutFromString(
		R"xml(
		<ScrollView layout_width="match_parent" layout_height="match_parent" touchdrag="true">
			<GridLayout columnMode="size" rowMode="size" columnWidth="200dp" rowHeight="200dp" layout_width="match_parent" layout_height="wrap_content" clip="false" />
		</ScrollView>
	)xml",
		this );

	mGridLayout = findByType<UIGridLayout>( UI_TYPE_GRID_LAYOUT );

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
		->setParent( mGridLayout );
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
