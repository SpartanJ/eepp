#ifndef EE_GAMINGCLAYERPROPERTIES_HPP
#define EE_GAMINGCLAYERPROPERTIES_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobjectobject.hpp>
#include <eepp/maps/maphelper.hpp>
#include <eepp/ui/uitable.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwindow.hpp>

using namespace EE::UI;

namespace EE { namespace Maps { namespace Private {

class MapEditor;

class MapObjectProperties {
  public:
	MapObjectProperties( GameObjectObject* Obj );

	virtual ~MapObjectProperties();

  protected:
	UITheme* mUITheme;
	UIWindow* mUIWindow;
	UITable* mGenGrid;
	GameObjectObject* mObj;
	UITextInput* mUIInput;
	UITextInput* mUIInput2;

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
