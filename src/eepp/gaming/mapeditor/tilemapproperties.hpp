#ifndef EE_GAMINGCMAPPROPERTIES_HPP
#define EE_GAMINGCMAPPROPERTIES_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/tilemap.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuigenericgrid.hpp>
#include <eepp/ui/cuitextinput.hpp>
#include <eepp/ui/cuislider.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class EE_API TileMapProperties {
	public:
		TileMapProperties( TileMap * Map );

		virtual ~TileMapProperties();

	protected:
		cUITheme *			mUITheme;
		cUIWindow *			mUIWindow;
		cUIGenericGrid *	mGenGrid;
		TileMap *				mMap;
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
