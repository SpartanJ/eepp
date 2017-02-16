#ifndef EE_UICUIDRAGABLE_H
#define EE_UICUIDRAGABLE_H

#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

class EE_API UIDragable : public UIControl {
	public:
		UIDragable( const UIControl::CreateParams& Params );

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		bool dragging() const;
		void dragging( const bool& dragging );

		const Vector2i& dragPoint() const;
		void dragPoint( const Vector2i& Point );

		virtual void update();

		bool dragEnable() const;
		void dragEnable( const bool& enable );

		void dragButton( const Uint32& Button );
		const Uint32& dragButton() const;
	protected:
		virtual ~UIDragable();

		Vector2i 	mDragPoint;
		Uint32 		mDragButton;

		virtual Uint32 onMouseDown( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseUp( const Vector2i& position, const Uint32 flags );

		virtual Uint32 OnDrag( const Vector2i& position );

		virtual Uint32 OnDragStart( const Vector2i& position );

		virtual Uint32 OnDragEnd( const Vector2i& position );
};

}}

#endif

