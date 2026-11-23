#ifndef EE_UI_UISVG_HPP
#define EE_UI_UISVG_HPP

#include <eepp/scene/mainthreadlifetime.hpp>
#include <eepp/ui/uiimage.hpp>
#include <optional>

namespace EE::Graphics {

class Image;

} // namespace EE::Graphics

namespace EE { namespace UI {

class EE_API UISvg : public UIImage {
  public:
	static UISvg* New();

	virtual ~UISvg();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	const std::string& getSvgXml() const;

  protected:
	virtual void onPixelDensityChange( Float oldDensity, Float newDensity );

	virtual void onUISceneNodeChange( UISceneNode* scene );

	UISvg();

	void onSizeChange();

	std::string mSvgXml;
	Uint64 mTaskId{ 0 };
	Uint64 mRasterGeneration{ 0 };
	std::optional<Scene::MainThreadLifetime<UISvg>> mRasterLifetime;

	void loadSvgXml( const pugi::xml_node& node );
	void scheduleRasterize();
	static std::shared_ptr<Image> rasterizeSvg( const std::string& svgXml,
												const Sizef& targetSize );
	void publishRaster( const Image& image, const Sizef& logicalSize, Uint64 generation );
	void clearThreadTag();
};

}} // namespace EE::UI

#endif
