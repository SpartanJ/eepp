#ifndef EE_GAMINGCMAPPROPERTIES_HPP
#define EE_GAMINGCMAPPROPERTIES_HPP

#include "base.hpp"
#include "../cmap.hpp"
#include "../../ui/cuiwindow.hpp"
#include "../../ui/cuigenericgrid.hpp"
#include "../../ui/cuitextinput.hpp"
#include "../../ui/cuislider.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class EE_API cMapProperties {
	public:
		cMapProperties( cMap * Map );

		virtual ~cMapProperties();

	protected:
		cUITheme *			mUITheme;
		cUIWindow *			mUIWindow;
		cUIGenericGrid *	mGenGrid;
		cMap *				mMap;
		cUIComplexControl *	mUIBaseColor;
		cUISlider *			mUIRedSlider;
		cUISlider *			mUIGreenSlider;
		cUISlider *			mUIBlueSlider;
		cUITextBox *		mUIRedTxt;
		cUITextBox *		mUIGreenTxt;
		cUITextBox *		mUIBlueTxt;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );

		void AddCellClick( const cUIEvent * Event );

		void RemoveCellClick( const cUIEvent * Event );

		void OnRedChange( const cUIEvent * Event );

		void OnGreenChange( const cUIEvent * Event );

		void OnBlueChange( const cUIEvent * Event );

		void CreateGridElems();

		void SaveProperties();

		void LoadProperties();

		cUIGridCell * CreateCell();
};

}}}

#endif
