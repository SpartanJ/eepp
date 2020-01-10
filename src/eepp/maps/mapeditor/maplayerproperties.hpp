#ifndef EE_GAMINGCLAYERPROPERTIES_HPP
#define EE_GAMINGCLAYERPROPERTIES_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/maphelper.hpp>
#include <eepp/maps/maplayer.hpp>
#include <eepp/ui/uitable.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwindow.hpp>

using namespace EE::UI;

namespace EE { namespace Maps { namespace Private {

class MapEditor;

class EE_API MapLayerProperties {
  public:
	typedef std::function<void()> RefreshLayerListCb;

	MapLayerProperties( MapLayer* Map, RefreshLayerListCb Cb = RefreshLayerListCb() );

	virtual ~MapLayerProperties();

  protected:
	UITheme* mUITheme;
	UIWindow* mUIWindow;
	UITable* mGenGrid;
	MapLayer* mLayer;
	UITextInput* mUIInput;
	RefreshLayerListCb mRefreshCb;

	void onWindowClose( const Event* Event );

	void onCancelClick( const Event* Event );

	void onOKClick( const Event* Event );

	void onAddCellClick( const Event* Event );

	void onRemoveCellClick( const Event* Event );

	void createGridElems();

	void saveProperties();

	void loadProperties();

	UITableCell* createCell();
};

}}} // namespace EE::Maps::Private

#endif
