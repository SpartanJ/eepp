#ifndef EE_UICUIMENUCHECKBOX_HPP
#define EE_UICUIMENUCHECKBOX_HPP

#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

class EE_API UIMenuCheckBox : public UIMenuItem {
	public:
		UIMenuCheckBox( UIMenuCheckBox::CreateParams& Params );

		virtual ~UIMenuCheckBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		const bool& Active() const;

		const bool& IsActive() const;

		void Active( const bool& active );

		void SwitchActive();

		virtual bool InheritsFrom( const Uint32 Type );
	protected:
		bool		mActive;
		UISkin *	mSkinActive;
		UISkin *	mSkinInactive;

		virtual Uint32 OnMouseUp( const Vector2i &Pos, const Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
