#ifndef EE_GAMINGCLAYERPROPERTIES_HPP
#define EE_GAMINGCLAYERPROPERTIES_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/maphelper.hpp>
#include <eepp/gaming/gameobjectobject.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uigenericgrid.hpp>
#include <eepp/ui/uitextinput.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class MapEditor;

class MapObjectProperties {
	public:
		MapObjectProperties( GameObjectObject * Obj );

		virtual ~MapObjectProperties();

	protected:
		UITheme *			mUITheme;
		UIWindow *			mUIWindow;
		UIGenericGrid *		mGenGrid;
		GameObjectObject *	mObj;
		UITextInput *		mUIInput;
		UITextInput *		mUIInput2;

		void onWindowClose( const UIEvent * Event );

		void onCancelClick( const UIEvent * Event );

		void onOKClick( const UIEvent * Event );

		void onAddCellClick( const UIEvent * Event );

		void onRemoveCellClick( const UIEvent * Event );

		void createGridElems();

		void saveProperties();

		void loadProperties();

		UIGridCell * createCell();
};

}}}

#endif
