#include "uitextureviewer.hpp"
#include <eepp/ui/uigridlayout.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI { namespace Tools {

namespace {

class UIWeakTexturePreview : public UIImage {
  public:
	static UIWeakTexturePreview* New( TextureWeakPtr texture ) {
		return eeNew( UIWeakTexturePreview, ( std::move( texture ) ) );
	}

	virtual void draw() {
		UINode::draw();

		if ( !mVisible || mAlpha == 0.f )
			return;

		TexturePtr texture = mTexture.lock();
		if ( !texture )
			return;

		const Sizef textureSize( texture->getPixelsSize() );
		if ( textureSize.x <= 0.f || textureSize.y <= 0.f )
			return;

		const Sizef availableSize( mSize.x - mPaddingPx.Left - mPaddingPx.Right,
								   mSize.y - mPaddingPx.Top - mPaddingPx.Bottom );
		Float scale = eemax( 0.f, eemin( 1.f, eemin( availableSize.x / textureSize.x,
													 availableSize.y / textureSize.y ) ) );
		Sizef destSize( ( textureSize * scale ).floor() );
		Vector2f position( std::trunc( mScreenPos.x ) + mPaddingPx.Left,
						   std::trunc( mScreenPos.y ) + mPaddingPx.Top );

		if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_CENTER )
			position.x += ( availableSize.x - destSize.x ) * 0.5f;
		else if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_RIGHT )
			position.x += availableSize.x - destSize.x;
		if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_CENTER )
			position.y += ( availableSize.y - destSize.y ) * 0.5f;
		else if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_BOTTOM )
			position.y += availableSize.y - destSize.y;

		const Color previousColor( texture->getColor() );
		texture->setColor( mColor );
		texture->draw( position, destSize );
		texture->setColor( previousColor );
	}

  protected:
	explicit UIWeakTexturePreview( TextureWeakPtr texture ) : mTexture( std::move( texture ) ) {}

	TextureWeakPtr mTexture;
};

} // namespace

UITextureViewer* UITextureViewer::New() {
	return eeNew( UITextureViewer, () );
}

UITextureViewer::~UITextureViewer() {}

UITextureViewer::UITextureViewer() : UIRelativeLayout( "textureviewer" ) {
	init();
}

void UITextureViewer::setImage( TexturePtr texture ) {
	UIImage* imageView = mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE );
	if ( imageView == nullptr )
		return;
	mSelectedTexture = std::move( texture );
	mImageLayout->setEnabled( true )->setVisible( true );
	imageView->setDrawable( mSelectedTexture );
}

void UITextureViewer::clearSelectedTexture() {
	if ( UIImage* imageView = mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE ) )
		imageView->setDrawable( DrawablePtr{} );
	mSelectedTexture.reset();
	mImageLayout->setEnabled( false )->setVisible( false );
}

void UITextureViewer::init() {
	mUISceneNode->loadLayoutFromString(
		R"xml(
		<ScrollView layout_width="match_parent" layout_height="match_parent" touchdrag="true">
			<GridLayout columnMode="size" rowMode="size" columnWidth="200dp" rowHeight="200dp" layout_width="match_parent" layout_height="wrap_content" clip="false" />
		</ScrollView>
		<RelativeLayout class="image_container" lw="mp" lh="mp" visible="false" enabled="false" background-color="black">
			<Image lw="mp" lh="mp" scaleType="fit_inside" gravity="center" enabled="false" lg="center" />
			<Image id="image_close" lw="22dp" lh="22dp" icon="icon(document-close, 22dp)" lg="top|right" enabled="false" />
		</RelativeLayout>
	)xml",
		this );

	mGridLayout = findByType<UIGridLayout>( UI_TYPE_GRID_LAYOUT );
	mImageLayout = findByClass<UIRelativeLayout>( "image_container" );
	auto hideImg = [this]( const Event* ) { clearSelectedTexture(); };
	mImageLayout->on( Event::MouseClick, hideImg );
	mImageLayout->on( Event::KeyDown, [hideImg]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
			hideImg( event );
	} );

	refreshTextures();
	subscribeScheduledUpdate();
}

static std::string getTextureDescription( Texture* tex ) {
	return String::format( "Name: %s\nSize: %dx%d", tex->getName().c_str(), tex->getWidth(),
						   tex->getHeight() );
}

void UITextureViewer::scheduledUpdate( const Time& ) {
	TextureFactory* textureFactory = TextureFactory::existsSingleton();
	if ( textureFactory && textureFactory->getLiveTextureGeneration() != mLastRegistryGeneration )
		refreshTextures();
}

void UITextureViewer::refreshTextures() {
	TextureFactory* textureFactory = TextureFactory::existsSingleton();
	if ( !textureFactory )
		return;

	const Uint64 registryGeneration = textureFactory->getLiveTextureGeneration();
	TextureRegistrySnapshot textures = textureFactory->snapshotTextures();
	for ( auto& texture : mTextures )
		texture.second.seen = false;

	for ( const TextureRegistryRecord& texture : textures ) {
		auto found = mTextures.find( texture.id.value() );
		if ( found == mTextures.end() ) {
			insertTexture( texture );
		} else {
			found->second.seen = true;
			found->second.texture = texture.texture;
		}
	}

	for ( auto texture = mTextures.begin(); texture != mTextures.end(); ) {
		if ( !texture->second.seen || texture->second.texture.expired() ) {
			if ( texture->second.preview )
				texture->second.preview->close();
			texture = mTextures.erase( texture );
		} else {
			++texture;
		}
	}

	mLastRegistryGeneration = registryGeneration;
}

void UITextureViewer::insertTexture( const TextureRegistryRecord& record ) {
	TexturePtr texture = record.texture.lock();
	if ( !texture )
		return;

	UIImage* img = UIWeakTexturePreview::New( record.texture );
	std::string uid(
		String::format( "texture-%llu", static_cast<unsigned long long>( record.id.value() ) ) );
	img->setScaleType( UIScaleType::FitInside )
		->setClasses( { "texture-preview", uid } )
		->setTooltipText( getTextureDescription( texture.get() ) )
		->setGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER )
		->setEnabled( true )
		->setParent( mGridLayout )
		->onClick( [this, weakTexture = record.texture]( auto ) {
			if ( TexturePtr selectedTexture = weakTexture.lock() )
				setImage( std::move( selectedTexture ) );
		} );
	mTextures.emplace( record.id.value(), TextureEntry{ record.texture, img, true } );
}

}}} // namespace EE::UI::Tools
