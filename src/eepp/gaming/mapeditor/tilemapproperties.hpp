#ifndef EE_GAMINGCMAPPROPERTIES_HPP
#define EE_GAMINGCMAPPROPERTIES_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/tilemap.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uigenericgrid.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uislider.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class EE_API TileMapProperties {
	public:
		TileMapProperties( TileMap * Map );

		virtual ~TileMapProperties();

	protected:
		UITheme *			mUITheme;
		UIWindow *			mUIWindow;
		UIGenericGrid *	mGenGrid;
		TileMap *				mMap;
		UIComplexControl *	mUIBaseColor;
		UISlider *			mUIRedSlider;
		UISlider *			mUIGreenSlider;
		UISlider *			mUIBlueSlider;
		UITextBox *		mUIRedTxt;
		UITextBox *		mUIGreenTxt;
		UITextBox *		mUIBlueTxt;

		void WindowClose( const UIEvent * Event );

		void CancelClick( const UIEvent * Event );

		void OKClick( const UIEvent * Event );

		void AddCellClick( const UIEvent * Event );

		void RemoveCellClick( const UIEvent * Event );

		void OnRedChange( const UIEvent * Event );

		void OnGreenChange( const UIEvent * Event );

		void OnBlueChange( const UIEvent * Event );

		void CreateGridElems();

		void SaveProperties();

		void LoadProperties();

		UIGridCell * CreateCell();
};

}}}

#endif
