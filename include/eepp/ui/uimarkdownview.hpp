#ifndef EE_UI_UIMARKDOWNVIEW_HPP
#define EE_UI_UIMARKDOWNVIEW_HPP

#include <eepp/ui/uilinearlayout.hpp>

namespace EE { namespace UI {

class EE_API UIMarkdownView : public UILinearLayout {
  public:
	static UIMarkdownView* New();

	UIMarkdownView();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void loadFromString( std::string_view markdown );

	virtual void loadFromXmlNode( const pugi::xml_node& node );
};

}} // namespace EE::UI

#endif
