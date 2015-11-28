#ifndef EE_UICUIDRAGABLE_H
#define EE_UICUIDRAGABLE_H

#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

class EE_API UIDragable : public UIControl {
	public:
		UIDragable( const UIControl::CreateParams& Params );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		bool Dragging() const;
		void Dragging( const bool& dragging );

		const Vector2i& DragPoint() const;
		void DragPoint( const Vector2i& Point );

		virtual void Update();

		bool DragEnable() const;
		void DragEnable( const bool& enable );

		void DragButton( const Uint32& Button );
		const Uint32& DragButton() const;
	protected:
		virtual ~UIDragable();

		Vector2i 	mDragPoint;
		Uint32 		mDragButton;

		virtual Uint32 OnMouseDown( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseUp( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnDrag( const Vector2i& Pos );

		virtual Uint32 OnDragStart( const Vector2i& Pos );

		virtual Uint32 OnDragEnd( const Vector2i& Pos );
};

}}

#endif

