#ifndef EE_UI_UIHTMLIMAGE_HPP
#define EE_UI_UIHTMLIMAGE_HPP

#include <eepp/ui/uiimage.hpp>

namespace EE { namespace UI {

class EE_API UIHTMLImage : public UIImage {
  public:
	static UIHTMLImage* New();

	virtual ~UIHTMLImage();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	virtual void draw();

	const std::string& getAlt() const;

	UIHTMLImage* setAlt( const std::string& alt );

  protected:
	UIHTMLImage();

	std::string mAlt;
};

}} // namespace EE::UI

#endif
