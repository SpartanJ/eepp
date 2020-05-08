#ifndef EE_GRAPHICS_UILAYOUT_HPP
#define EE_GRAPHICS_UILAYOUT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UILayout : public UIWidget {
  public:
	static UILayout* New();

	UILayout();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

  protected:
	friend class UISceneNode;

	UILayout( const std::string& tag );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	virtual void onParentSizeChange( const Vector2f& SizeChange );

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onLayoutUpdate();

	virtual void tryUpdateLayout();

	virtual void updateLayout();

	virtual void updateLayoutTree();

	void setLayoutDirty();

	std::unordered_set<UILayout*> mLayouts;
	bool mDirtyLayout;
};

}} // namespace EE::UI

#endif
