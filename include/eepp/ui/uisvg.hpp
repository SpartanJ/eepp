#ifndef EE_UI_UISVG_HPP
#define EE_UI_UISVG_HPP

#include <eepp/ui/uiimage.hpp>

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
	UISvg();

	void onSizeChange();

	std::string mSvgXml;
	Uint64 mTaskId{ 0 };

	void loadSvgXml( const pugi::xml_node& node );
	void scheduleRasterize();
	void rasterizeSvg( const std::string& svgXml );
	void clearThreadTag();
};

}} // namespace EE::UI

#endif
