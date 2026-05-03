#include "utest.h"

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Models;

UTEST( UIDropDownModelList, basicFunctionality ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UIDropDownModelList Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5 ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UISceneNode* sceneNode = app.getUI();

	UIDropDownModelList* dropDown = UIDropDownModelList::New();
	dropDown->setParent( sceneNode );

	std::vector<std::string> items = { "Item 1", "Item 2", "Item 3" };
	auto model = ItemListOwnerModel<std::string>::create( items );
	dropDown->setModel( model );

	// Model should be set
	EXPECT_TRUE( dropDown->getModel() == model );

	// Items count should match
	EXPECT_EQ( dropDown->getListView()->getModel()->rowCount(), 3ul );

	// Max visible items
	dropDown->setMaxNumVisibleItems( 2 );
	EXPECT_EQ( dropDown->getMaxNumVisibleItems(), 2ul );
}
