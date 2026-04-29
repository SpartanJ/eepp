#include <eepp/ee.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>

using namespace EE::UI;
using namespace EE::UI::Models;

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 640, 480, "eepp - UIDropDownModelList Example" } );
	app.getUI()->loadLayoutFromString( R"xml(
			<LinearLayout layout_width="match_parent"
						  layout_height="match_parent"
						  orientation="vertical"
						  padding="16dp">
				<TextView layout_width="match_parent"
						  layout_height="wrap_content"
						  text="Model-Based Dropdown Example"
						  margin_bottom="16dp" />

				<DropDownModelList id="dropdown"
								   layout_width="250dp"
								   layout_height="wrap_content"
								   pop-up-to-root="true"
								   max-visible-items="4" />
			</LinearLayout>
	)xml" );

	if ( !app.getWindow()->isOpen() )
		return EXIT_FAILURE;

	UIDropDownModelList* dropDown = app.getUI()->find<UIDropDownModelList>( "dropdown" );
	if ( dropDown ) {
		std::vector<std::string> options = { "Option 1: OpenGL",	  "Option 2: Vulkan",
											 "Option 3: Direct3D 11", "Option 4: Direct3D 12",
											 "Option 5: Metal",		  "Option 6: WebGL",
											 "Option 7: Software" };

		auto model = ItemListOwnerModel<std::string>::create( options );
		dropDown->setModel( model );
		dropDown->addEventListener( Event::OnItemSelected, []( const Event* event ) {
			UIDropDownModelList* dropDown = event->getNode()->asType<UIDropDownModelList>();
			ModelIndex index = dropDown->getListView()->getSelection().first();
			if ( index.isValid() ) {
				String text = dropDown->getListView()->getModel()->data( index ).toString();
				Log::info( "Selected item index: %d, value: %s", (int)index.row(), text.c_str() );
			}
		} );
	}

	return app.run();
}
