#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIMenuItem : public UIPushButton {
  public:
	typedef std::function<bool( UIMenuItem* item )> OnShouldCloseCb;

	static UIMenuItem* New();

	UIMenuItem();

	virtual ~UIMenuItem();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual UIMenuItem* setShortcutText( const String& text );

	UITextView* getShortcutView() const;

	virtual UIWidget* getExtraInnerWidget() const;

	OnShouldCloseCb getOnShouldCloseCb() const;

	UIMenuItem* setOnShouldCloseCb( const OnShouldCloseCb& onShouldCloseCb );

  protected:
	UITextView* mShortcutView;
	OnShouldCloseCb mOnShouldCloseCb;

	explicit UIMenuItem( const std::string& tag );

	virtual void onSizeChange();

	virtual Uint32 onMouseOver( const Vector2i& pos, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& pos, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& pos, const Uint32& flags );

	void createShortcutView();
};

}} // namespace EE::UI

#endif
