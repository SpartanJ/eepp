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
	UnorderedSet<Int64> guiPids;
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

UTEST( EProcProcessFilter, ClassificationDoesNotDependOnUnixUids ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto model = ProcessModel::create( app.getUI() );
	ProcessInfo own;
	own.pid = 1;
	own.uid = 0;
	own.ownedByCurrentUser = true;
	own.userProcess = true;
	ProcessInfo service;
	service.pid = 2;
	service.uid = 1000;
	service.systemProcess = true;
	model->applySnapshot( { own, service }, {} );

	model->setFilter( ProcessModel::OwnProcesses );
	EXPECT_EQ( model->visibleCount(), 1u );
	EXPECT_TRUE( model->rowForPid( 1 ) >= 0 );
	model->setFilter( ProcessModel::SystemProcesses );
	EXPECT_EQ( model->visibleCount(), 1u );
	EXPECT_TRUE( model->rowForPid( 2 ) >= 0 );
	model->setFilter( ProcessModel::UserProcesses );
	EXPECT_EQ( model->visibleCount(), 1u );
	EXPECT_TRUE( model->rowForPid( 1 ) >= 0 );
}

UTEST( EProcProcessFilter, UnavailableMemoryAndPidReuse ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto model = ProcessModel::create( app.getUI() );
	ProcessInfo first;
	first.pid = 9;
	first.startTime = 10;
	EXPECT_EQ( first.getMemoryForSort(), -1 );
	EXPECT_TRUE( first.formatMemory().empty() );
	first.vmRSS = 1024;
	EXPECT_EQ( first.getMemoryForSort(), 1024 );
	first.vmURSS = 512;
	EXPECT_EQ( first.getMemoryForSort(), 512 );
	model->applySnapshot( { first }, {} );
	ProcessInfo reused = first;
	reused.startTime = 20;
	reused.userUsage = 150;
	model->applySnapshot( { reused }, {} );
	EXPECT_EQ( model->visibleCount(), 2u );
	EXPECT_EQ( model->getProcessByRow( 0 )->getCpuForSort(), 150 );
	EXPECT_EQ( model->getProcessByRow( 1 )->status, ProcessStatus::Ended );
}
