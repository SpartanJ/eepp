#ifndef EE_GAMINGCMAPPROPERTIES_HPP
#define EE_GAMINGCMAPPROPERTIES_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/tilemap.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uitable.hpp>
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
		UITable *		mGenGrid;
		TileMap *			mMap;
		UIWidget *	mUIBaseColor;
		UISlider *			mUIRedSlider;
		UISlider *			mUIGreenSlider;
		UISlider *			mUIBlueSlider;
		UITextView *			mUIRedTxt;
		UITextView *			mUIGreenTxt;
		UITextView *			mUIBlueTxt;

		void onWindowClose( const UIEvent * Event );

		void onCancelClick( const UIEvent * Event );

		void onOKClick( const UIEvent * Event );

		void onAddCellClick( const UIEvent * Event );

		void onRemoveCellClick( const UIEvent * Event );

		void onRedChange( const UIEvent * Event );

		void onGreenChange( const UIEvent * Event );

		void onBlueChange( const UIEvent * Event );

		void createGridElems();

		void saveProperties();

		void loadProperties();

		UITableCell * createCell();
};

}}}

#endif
