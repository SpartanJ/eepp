#include "../../tools/eproc/process_model.hpp"
#include "utest.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>

using namespace eproc;

UTEST( EProcProcessFilter, ProgramsOnlyRequiresGuiWindow ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto model = ProcessModel::create( app.getUI() );
	UnorderedSet<long> guiPids;
	guiPids.insert( 3 );
	guiPids.insert( 4 );
	model->setGuiWindowPids( std::move( guiPids ) );
	model->setFilter( ProcessModel::ProgramsOnly );

	ProcessInfo fish;
	fish.pid = 1;
	fish.name = "fish";
	fish.ttyNr = 123;
	ProcessInfo ls;
	ls.pid = 2;
	ls.name = "ls";
	ls.ttyNr = 123;
	ProcessInfo gui;
	gui.pid = 3;
	gui.name = "editor";
	gui.ttyNr = 123;
	ProcessInfo tray;
	tray.pid = 4;
	tray.name = "tray app";
	model->applySnapshot( { fish, ls, gui, tray }, {} );

	EXPECT_EQ( model->visibleCount(), 2u );
	EXPECT_EQ( model->rowForPid( 1 ), -1 );
	EXPECT_EQ( model->rowForPid( 2 ), -1 );
	EXPECT_TRUE( model->rowForPid( 3 ) >= 0 );
	EXPECT_TRUE( model->rowForPid( 4 ) >= 0 );
}
