#ifndef EE_WINDOWCVIEW_H
#define EE_WINDOWCVIEW_H

#include "base.hpp"

namespace EE { namespace Window {

/** @brief The class defines a view like a 2D camera ( position, size, move, scale ). \nBasically is a 2D proyection in pixels seted over a viewport. */
class EE_API cView {
	public:
		cView();

		cView( const eeInt& X, const eeInt& Y, const eeInt& Width, const eeInt& Height );

		cView( const eeRecti& View );

		~cView();

		/** Offset the position */
		void Move( const eeInt& OffsetX, const eeInt& OffsetY );

		/** Offset the position */
		void Move( const eeVector2i& Offset );

		/** Scale the current view (from center) */
		void Scale( const eeFloat& Factor );

		/** @return The center position of the view */
		eeVector2i Center() const;

		/** Set the center position of the view ( will move it if is needed ) */
		void Center( const eeVector2i& Center );

		/** @return The current view ( Left = X, Right = Width, Top = Y, Bottom = Height ) */
		eeRecti GetView() const { return mView; }

		/** Set a new position to the view */
		void SetPosition( const eeInt& X, const eeInt& Y );

		/** Set a new size to the view */
		void SetSize( const eeInt& Width, const eeInt& Height );

		/** Creates a new view */
		void SetView( const eeInt& X, const eeInt& Y, const eeInt& Width, const eeInt& Height );
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
