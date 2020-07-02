#ifndef EE_UICUIMENUCHECKBOX_HPP
#define EE_UICUIMENUCHECKBOX_HPP

#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

class EE_API UIMenuCheckBox : public UIMenuItem {
  public:
	static UIMenuCheckBox* New();

	UIMenuCheckBox();

	virtual ~UIMenuCheckBox();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	const bool& isActive() const;

	UIMenuCheckBox* setActive( const bool& active );

	void switchActive();

  protected:
	bool mActive;
	UISkin* mSkinActive;
	UISkin* mSkinInactive;

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual void onStateChange();
};

}} // namespace EE::UI

#endif
