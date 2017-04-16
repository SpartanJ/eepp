#ifndef EE_WINDOWCVIEW_H
#define EE_WINDOWCVIEW_H

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

/** @brief The class defines a view like a 2D camera ( position, size, move, scale ). Basically is a 2D proyection in pixels seted over a viewport. */
class EE_API View {
	public:
		View();

		View( const int& X, const int& Y, const int& Width, const int& Height );

		View( const Rect& View );

		~View();

		/** Offset the position */
		void move( const int& OffsetX, const int& OffsetY );

		/** Offset the position */
		void move( const Vector2i& Offset );

		/** Scale the current view (from center) */
		void scale( const Float& Factor );

		/** Scale the current view (from center) */
		void scale( const Vector2f& Factor );

		/** @return The center position of the view */
		Vector2i getCenter() const;

		/** Set the center position of the view ( will move it if is needed ) */
		void setCenter( const Vector2i& center );

		/** @return The current view ( Left = X, Right = Width, Top = Y, Bottom = Height ) */
		Rect getView() const { return mView; }

		/** Set a new position to the view */
		void setPosition( const int& X, const int& Y );

		/** Set a new size to the view */
		void setSize( const int& Width, const int& Height );

		/** Creates a new view */
		void setView( const int& X, const int& Y, const int& Width, const int& Height );
	private:
		friend class Window;

		bool mNeedUpdate;
		Rect mView;
		Vector2f mCenter;

		void calcCenter();

		bool needUpdate() const;
};

}}

#endif
