#ifndef EE_WINDOWCVIEW_H
#define EE_WINDOWCVIEW_H

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

/** @brief The class defines a view like a 2D camera ( position, size, move, scale ). Basically is a 2D proyection in pixels seted over a viewport. */
class EE_API cView {
	public:
		cView();

		cView( const int& X, const int& Y, const int& Width, const int& Height );

		cView( const Recti& View );

		~cView();

		/** Offset the position */
		void Move( const int& OffsetX, const int& OffsetY );

		/** Offset the position */
		void Move( const Vector2i& Offset );

		/** Scale the current view (from center) */
		void Scale( const Float& Factor );

		/** Scale the current view (from center) */
		void Scale( const Vector2f& Factor );

		/** @return The center position of the view */
		Vector2i Center() const;

		/** Set the center position of the view ( will move it if is needed ) */
		void Center( const Vector2i& Center );

		/** @return The current view ( Left = X, Right = Width, Top = Y, Bottom = Height ) */
		Recti GetView() const { return mView; }

		/** Set a new position to the view */
		void SetPosition( const int& X, const int& Y );

		/** Set a new size to the view */
		void SetSize( const int& Width, const int& Height );

		/** Creates a new view */
		void SetView( const int& X, const int& Y, const int& Width, const int& Height );
	private:
		friend class cWindow;

		bool mNeedUpdate;
		Recti mView;
		Vector2f mCenter;

		void CalcCenter();

		bool NeedUpdate() const;
};

}}

#endif
