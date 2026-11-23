#include <eepp/ui/uisvg.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <memory>

namespace EE { namespace UI {

namespace {

class XmlStringWriter : public pugi::xml_writer {
  public:
	std::string result;
	virtual void write( const void* data, size_t size ) override {
		result.append( static_cast<const char*>( data ), size );
	}
};

} // namespace

UISvg* UISvg::New() {
	return eeNew( UISvg, () );
}

UISvg::UISvg() : UIImage( "svg" ) {
	mFlags |= UI_LOADS_ITS_CHILDREN;
}

void UISvg::onPixelDensityChange( Float oldDensity, Float newDensity ) {
	UIImage::onPixelDensityChange( oldDensity, newDensity );
	if ( mSvgXml.empty() )
		return;
	++mRasterGeneration;
	clearThreadTag();
	// Read the final target size after the scene has reapplied styles and settled layout.
	debounce( [this] { scheduleRasterize(); }, Time::Zero, (UintPtr)this );
}

UISvg::~UISvg() {
	if ( mRasterLifetime )
		mRasterLifetime->invalidate();
	clearThreadTag();
}

void UISvg::onUISceneNodeChange( UISceneNode* scene ) {
	UIImage::onUISceneNodeChange( scene );
	if ( mRasterLifetime )
		mRasterLifetime->setDispatcher( scene );
}

Uint32 UISvg::getType() const {
	return UI_TYPE_SVG;
}

bool UISvg::isType( const Uint32& type ) const {
	return UISvg::getType() == type ? true : UIImage::isType( type );
}

void UISvg::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();
	UIWidget::loadFromXmlNode( node );
	endAttributesTransaction();

	loadSvgXml( node );

	scheduleRasterize();
}

void UISvg::loadSvgXml( const pugi::xml_node& node ) {
	XmlStringWriter writer;
	node.print( writer );
	mSvgXml = writer.result;
}

void UISvg::scheduleRasterize() {
	if ( mSvgXml.empty() )
		return;

	auto size = getPixelsSize();
	if ( size.getWidth() <= 0.f || size.getHeight() <= 0.f )
		return;
	const Float density = getUISceneNode()->getPixelDensity();
	const Sizef logicalSize = size / density;
	const Uint64 generation = ++mRasterGeneration;

	if ( !getUISceneNode()->hasThreadPool() ) {
		auto densityContext = PixelDensity::makeCurrent( density );
		if ( auto image = rasterizeSvg( mSvgXml, size ) ) {
			runOnMainThread( [this, generation, logicalSize, image = std::move( image )] {
				publishRaster( *image, logicalSize, generation );
			} );
		}
		return;
	}

	clearThreadTag();
	if ( !mRasterLifetime )
		mRasterLifetime.emplace( this, getUISceneNode() );

	std::string svgXml( mSvgXml );
	mTaskId = getUISceneNode()->getThreadPool()->run(
		[lifetime = mRasterLifetime->weakHandle(), svgXml = std::move( svgXml ), size, density,
		 generation, logicalSize] {
			auto densityContext = PixelDensity::makeCurrent( density );
			if ( auto image = rasterizeSvg( svgXml, size ) ) {
				lifetime.run( [generation, logicalSize, image = std::move( image )]( UISvg* svg ) {
					svg->publishRaster( *image, logicalSize, generation );
				} );
			}
		},
		{}, (Uint64)this );
}

std::shared_ptr<Image> UISvg::rasterizeSvg( const std::string& svgXml, const Sizef& targetSize ) {
	pugi::xml_document document;
	std::string rasterXml;
	if ( document.load_buffer( svgXml.data(), svgXml.size() ) ) {
		pugi::xml_node root = document.document_element();
		auto setRasterDimension = [&]( const char* name, Float value ) {
			pugi::xml_attribute attribute = root.attribute( name );
			if ( !attribute )
				attribute = root.append_attribute( name );
			if ( attribute.as_string()[0] == '\0' ||
				 std::string_view( attribute.as_string() ).find( '%' ) != std::string_view::npos )
				attribute.set_value( value );
		};
		setRasterDimension( "width", targetSize.getWidth() );
		setRasterDimension( "height", targetSize.getHeight() );
		XmlStringWriter writer;
		document.print( writer );
		rasterXml = std::move( writer.result );
	}
	const std::string& source = rasterXml.empty() ? svgXml : rasterXml;
	Image::FormatConfiguration format;
	int width = 0, height = 0, channels = 0;
	if ( Image::getInfoFromMemory( reinterpret_cast<const Uint8*>( source.data() ), source.size(),
								   &width, &height, &channels ) &&
		 width > 0 && height > 0 ) {
		format.svgScale( eemax( targetSize.x / width, targetSize.y / height ) );
	}
	Image image( reinterpret_cast<const Uint8*>( source.data() ), source.size(), 0, format );
	return image.getPixelsPtr() ? std::make_shared<Image>( std::move( image ) ) : nullptr;
}

void UISvg::publishRaster( const Image& image, const Sizef& logicalSize, Uint64 generation ) {
	if ( generation != mRasterGeneration || isClosing() )
		return;
	// The widget's window may have changed since decoding began. Upload only for a current
	// result, using the graphics context that owns the widget now.
	auto context = getUISceneNode()->makeCurrent();
	TexturePtr texture = TextureFactory::instance()->loadFromPixels(
		image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels() );

	if ( !texture )
		return;

	SpritePtr sprite = Sprite::New();
	// TextureRegion destination sizes are density-independent and get converted back to pixels by
	// the drawable. The SVG texture is already rasterized at the widget's physical pixel size.
	sprite->createStatic( std::move( texture ), logicalSize );
	setDrawable( std::move( sprite ) );
}

void UISvg::onSizeChange() {
	UIImage::onSizeChange();

	auto size = getPixelsSize();
	if ( size.getWidth() <= 0.f || size.getHeight() <= 0.f || mSvgXml.empty() )
		return;

	debounce( [this] { scheduleRasterize(); }, Milliseconds( 150 ), (UintPtr)this );
}

void UISvg::clearThreadTag() {
	if ( mTaskId != 0 && getUISceneNode()->hasThreadPool() ) {
		getUISceneNode()->getThreadPool()->removeWithTag( (Uint64)this );
		mTaskId = 0;
	}
}

const std::string& UISvg::getSvgXml() const {
	return mSvgXml;
}

}} // namespace EE::UI
