#ifndef EE_UI_UIMENURADIOBUTTON_HPP
#define EE_UI_UIMENURADIOBUTTON_HPP

#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

class EE_API UIMenuRadioButton : public UIMenuItem {
  public:
	static UIMenuRadioButton* New();

	UIMenuRadioButton();

	virtual ~UIMenuRadioButton();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	const bool& isActive() const;

	void setActive( const bool& active );

	void switchActive();

  protected:
	bool mActive;
	UISkin* mSkinActive;
	UISkin* mSkinInactive;

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual void onStateChange();
};

}} // namespace EE::UI

#endif // EE_UI_UIMENURADIOBUTTON_HPP
