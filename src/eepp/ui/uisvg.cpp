#include <eepp/ui/uisvg.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/ui/uiscenenode.hpp>

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
		rasterizeSvg( mSvgXml );
		return;
	}

	clearThreadTag();

	std::string svgXml( mSvgXml );
	mTaskId = getUISceneNode()->getThreadPool()->run(
		[this, svgXml = std::move( svgXml )] { rasterizeSvg( svgXml ); }, {}, (Uint64)this );
}

void UISvg::rasterizeSvg( const std::string& svgXml ) {
	Texture* texture = TextureFactory::instance()->loadFromMemory(
		(const unsigned char*)svgXml.data(), svgXml.size() );

	if ( !texture )
		return;

	Sprite* sprite = Sprite::New();
	sprite->createStatic( texture );
	sprite->setAsTextureOwner( true );
	sprite->setAsTextureRegionOwner( true );

	runOnMainThread( [this, sprite] { setDrawable( sprite, true ); } );
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
