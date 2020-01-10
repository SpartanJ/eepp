#ifndef EE_UICUILISTBOXITEM_HPP
#define EE_UICUILISTBOXITEM_HPP

#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uitextview.hpp>

namespace EE { namespace UI {

class UIListBox;

class EE_API UIListBoxItem : public UITextView {
  public:
	static UIListBoxItem* New();

	static UIListBoxItem* NewWithTag( const std::string& tag );

	UIListBoxItem();

	UIListBoxItem( const std::string& tag );

	virtual ~UIListBoxItem();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	bool isSelected() const;

	void unselect();

	void select();

  protected:
	friend class UIItemContainer<UIListBox>;

	virtual void onStateChange();

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );
};

}} // namespace EE::UI

#endif
