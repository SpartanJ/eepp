#ifndef EE_WINDOWCVIEW_H
#define EE_WINDOWCVIEW_H

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

/** @brief The class defines a view like a 2D camera ( position, size, move, scale ). Basically is a 2D proyection in pixels seted over a viewport. */
class EE_API cView {
	public:
		cView();

		cView( const int& X, const int& Y, const int& Width, const int& Height );

		cView( const eeRecti& View );

		~cView();

		/** Offset the position */
		void Move( const int& OffsetX, const int& OffsetY );

		/** Offset the position */
		void Move( const eeVector2i& Offset );

		/** Scale the current view (from center) */
		void Scale( const Float& Factor );

		/** Scale the current view (from center) */
		void Scale( const eeVector2f& Factor );

		/** @return The center position of the view */
		eeVector2i Center() const;

		/** Set the center position of the view ( will move it if is needed ) */
		void Center( const eeVector2i& Center );

		/** @return The current view ( Left = X, Right = Width, Top = Y, Bottom = Height ) */
		eeRecti GetView() const { return mView; }

		/** Set a new position to the view */
		void SetPosition( const int& X, const int& Y );

		/** Set a new size to the view */
		void SetSize( const int& Width, const int& Height );

		/** Creates a new view */
		void SetView( const int& X, const int& Y, const int& Width, const int& Height );
	private:
		friend class cWindow;

		bool mNeedUpdate;
		eeRecti mView;
		eeVector2f mCenter;

		void CalcCenter();

		bool NeedUpdate() const;
};

}}

#endif
