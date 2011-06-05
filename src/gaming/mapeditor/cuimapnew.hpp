#ifndef EE_GAMINGCUIMAPNEW_HPP
#define EE_GAMINGCUIMAPNEW_HPP

#include "base.hpp"
#include "../../ui/cuiwindow.hpp"
#include "cuimap.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class cUIMapNew {
	public:
		cUIMapNew( cUIMap * Map, cb::Callback0<void> NewMapCb = cb::Callback0<void>() );

		virtual ~cUIMapNew();
	protected:
		cUITheme *			mTheme;
		cUIWindow *			mUIWindow;
		cUIMap *			mUIMap;
		cUISpinBox *		mUIMapWidth;
		cUISpinBox *		mUIMapHeight;
		cUISpinBox *		mUIMapTWidth;
		cUISpinBox *		mUIMapTHeight;
		cUISpinBox *		mUIMapMaxLayers;
		cb::Callback0<void> mNewMapCb;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );
};

}}}

#endif
