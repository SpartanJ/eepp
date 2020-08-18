#ifndef EE_GAMINGCMAPPROPERTIES_HPP
#define EE_GAMINGCMAPPROPERTIES_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwidgettable.hpp>
#include <eepp/ui/uiwindow.hpp>

using namespace EE::UI;

namespace EE { namespace Maps { namespace Private {

class EE_API TileMapProperties {
  public:
	TileMapProperties( TileMap* Map );

	virtual ~TileMapProperties();

  protected:
	UITheme* mUITheme;
	UIWindow* mUIWindow;
	UIWidgetTable* mGenGrid;
	TileMap* mMap;
	UIWidget* mUIBaseColor;
	UISlider* mUIRedSlider;
	UISlider* mUIGreenSlider;
	UISlider* mUIBlueSlider;
	UITextView* mUIRedTxt;
	UITextView* mUIGreenTxt;
	UITextView* mUIBlueTxt;

	void onWindowClose( const Event* Event );

	void onCancelClick( const Event* Event );

	void onOKClick( const Event* Event );

	void onAddCellClick( const Event* Event );

	void onRemoveCellClick( const Event* Event );

	void onRedChange( const Event* Event );

	void onGreenChange( const Event* Event );

	void onBlueChange( const Event* Event );

	void createGridElems();

	void saveProperties();

	void loadProperties();

	UIWidgetTableRow* createCell();
};

}}} // namespace EE::Maps::Private

#endif
