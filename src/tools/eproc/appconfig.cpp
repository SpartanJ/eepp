#include "appconfig.hpp"

#include <eepp/system/filesystem.hpp>

using namespace EE;
using namespace EE::System;

namespace eproc {

AppConfig::AppConfig( std::string configPath ) : mConfigPath( std::move( configPath ) ) {
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mState.path( mConfigPath + "state.cfg" );
}

void AppConfig::load() {
	mState.loadFromFile( mState.path() );

	windowState.size.setWidth( mState.getValueI( "window", "width", windowState.size.getWidth() ) );
	windowState.size.setHeight(
		mState.getValueI( "window", "height", windowState.size.getHeight() ) );
	windowState.position.x = mState.getValueI( "window", "x", windowState.position.x );
	windowState.position.y = mState.getValueI( "window", "y", windowState.position.y );
	windowState.displayIndex =
		mState.getValueI( "window", "display_index", windowState.displayIndex );
	windowState.maximized = mState.getValueB( "window", "maximized", windowState.maximized );
	processTableState = mState.getValue( "process_table", "state", processTableState );
}

bool AppConfig::saveWindowState() {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;

	mState.setValueI( "window", "width", windowState.size.getWidth() );
	mState.setValueI( "window", "height", windowState.size.getHeight() );
	mState.setValueI( "window", "x", windowState.position.x );
	mState.setValueI( "window", "y", windowState.position.y );
	mState.setValueI( "window", "display_index", windowState.displayIndex );
	mState.setValueB( "window", "maximized", windowState.maximized );
	mState.setValue( "process_table", "state", processTableState );
	return mState.writeFile();
}

void AppConfig::captureWindowState( EE::Window::Window* window ) {
	if ( !window )
		return;

	windowState.size = Sys::getPlatformType() == Sys::PlatformType::macOS
						   ? window->getSizeInScreenCoordinates()
						   : window->getLastWindowedSizeInScreenCoordinates();
	windowState.position = window->getPosition();
	windowState.position.x = eemax( 0, windowState.position.x );
	windowState.position.y = eemax( 0, windowState.position.y );
	windowState.displayIndex = window->getCurrentDisplayIndex();
	windowState.maximized = window->isMaximized();
}

} // namespace eproc
