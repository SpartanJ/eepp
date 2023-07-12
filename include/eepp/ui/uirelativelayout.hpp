#ifndef UI_UIRELATIVELAYOUT_HPP
#define UI_UIRELATIVELAYOUT_HPP

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIRelativeLayout : public UILayout {
  public:
	static UIRelativeLayout* New();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UIRelativeLayout* add( UIWidget* widget );

	virtual void updateLayout();

  protected:
	UIRelativeLayout( const std::string& tagName );

	UIRelativeLayout();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void fixChildPos( UIWidget* widget );

	void fixChildSize( UIWidget* widget );
};

}} // namespace EE::UI

#endif
