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

UISvg::~UISvg() {
	clearThreadTag();
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

	if ( !getUISceneNode()->hasThreadPool() ) {
		rasterizeSvg( mSvgXml, size );
		return;
	}

	clearThreadTag();

	std::string svgXml( mSvgXml );
	mTaskId = getUISceneNode()->getThreadPool()->run(
		[this, svgXml = std::move( svgXml ), size] { rasterizeSvg( svgXml, size ); }, {},
		(Uint64)this );
}

void UISvg::rasterizeSvg( const std::string& svgXml, const Sizef& targetSize ) {
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
	TexturePtr texture = TextureFactory::instance()->loadFromMemory(
		(const unsigned char*)source.data(), source.size() );

	if ( !texture )
		return;

	SpritePtr sprite = Sprite::New();
	// TextureRegion destination sizes are density-independent and get converted back to pixels by
	// the drawable. The SVG texture is already rasterized at the widget's physical pixel size.
	sprite->createStatic( std::move( texture ), PixelDensity::pxToDp( targetSize ) );

	runOnMainThread( [this, sprite = std::move( sprite )]() mutable {
		if ( sprite )
			setDrawable( std::move( sprite ) );
	} );
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
