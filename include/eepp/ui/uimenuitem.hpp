#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

enum class MenuRole : Uint8 { NoRole, About, Preferences, Quit };

class EE_API UIMenuItem : public UIPushButton {
  public:
	typedef std::function<bool( UIMenuItem* item )> OnShouldCloseCb;

	static UIMenuItem* New();

	virtual ~UIMenuItem();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void activate();

	virtual UIMenuItem* setShortcutText( const String& text );

	const KeyBindings::Shortcut& getShortcut() const;

	UITextView* getShortcutView() const;

	virtual UIWidget* getExtraInnerWidget() const;

	OnShouldCloseCb getOnShouldCloseCb() const;

	UIMenuItem* setOnShouldCloseCb( const OnShouldCloseCb& onShouldCloseCb );

	MenuRole getMenuRole() const;

	UIMenuItem* setMenuRole( MenuRole role );

  protected:
	UITextView* mShortcutView;
	OnShouldCloseCb mOnShouldCloseCb;
	mutable KeyBindings::Shortcut mShortcut;
	MenuRole mMenuRole{ MenuRole::NoRole };

	UIMenuItem();

	explicit UIMenuItem( const std::string& tag );

	virtual void onSizeChange();

	virtual void onStateChange();

	virtual void onLayoutUpdate();

	virtual Uint32 onMouseOver( const Vector2i& pos, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& pos, const Uint32& flags );

	void createShortcutView();

	void refreshShortcut();
};

}} // namespace EE::UI

#endif
