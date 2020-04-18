#ifndef UI_UIRELATIVELAYOUT_HPP
#define UI_UIRELATIVELAYOUT_HPP

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIRelativeLayout : public UILayout {
  public:
	static UIRelativeLayout* New();

	UIRelativeLayout();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UIRelativeLayout* add( UIWidget* widget );

  protected:
	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	virtual void onChildCountChange( Node * child, const bool& removed );

	virtual void onParentSizeChange( const Vector2f& SizeChange );

	void fixChilds();

	void fixChildPos( UIWidget* widget );

	void fixChildSize( UIWidget* widget );
};

}} // namespace EE::UI

#endif
