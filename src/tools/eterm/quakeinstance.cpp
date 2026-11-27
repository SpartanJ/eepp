#include "quakeinstance.hpp"

#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window/input.hpp>

namespace eterm {

using namespace EE::System;
using EE::Window::Input;

QuakeInstance::Start QuakeInstance::start() {
	// Use the default configuration directory for the lock, independent of --profile.
	// Keep ownership across socket setup and stale socket recovery.
	auto directory = Sys::getConfigPath( "eterm" );
	if ( directory.empty() ||
		 ( !FileSystem::isDirectory( directory ) && !FileSystem::makeDir( directory, true ) &&
		   !FileSystem::isDirectory( directory ) ) )
		return Start::Failed;
	const auto endpoint = "eterm-quake:" + directory;
	FileSystem::dirAddSlashAtEnd( directory );
	const auto lockPath = directory + "quake-instance.lock";

	Clock waiting;
	do {
		const auto ownership = mLock.tryLock( lockPath );
		if ( ownership == FileLock::Result::Acquired ) {
			const auto listening = mIPC.listen( endpoint, [this]( const void*, std::size_t size ) {
				if ( size != 0 )
					return;
				const bool queued = mToggleRequested.exchange( true );
				if ( !queued && mWindowReady.load() )
					Input::tryWakeUpEventLoop();
			} );
			return listening == IPC::Status::Done ? Start::Owner : Start::Failed;
		}
		if ( ownership != FileLock::Result::Busy )
			return Start::Failed;
		const auto delivery = IPC::send( endpoint, std::string_view{}, Milliseconds( 100 ) );
		if ( delivery == IPC::Status::Done )
			return Start::Forwarded;
		if ( delivery != IPC::Status::NotFound && delivery != IPC::Status::Disconnected &&
			 delivery != IPC::Status::Timeout )
			return Start::Failed;
		Sys::sleep( Milliseconds( 20 ) );
	} while ( waiting.getElapsedTime() < Seconds( 2 ) );
	return Start::Failed;
}

void QuakeInstance::windowReady() {
	mWindowReady.store( true );
}

bool QuakeInstance::takeToggleRequest() {
	return mToggleRequested.exchange( false );
}

void QuakeInstance::stopListening() {
	mIPC.close();
}

} // namespace eterm
