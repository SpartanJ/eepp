#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX

#include "devicemonitor.hpp"
#include "places.hpp"

#include <algorithm>
#include <eepp/core/small_vector.hpp>
#include <eepp/system/sys.hpp>
#include <fcntl.h>
#include <poll.h>
#include <string_view>
#include <unistd.h>

using namespace EE;
using namespace EE::System;

namespace eefm {

namespace {

struct UDisksClient;
struct UDisksObject;
struct UDisksBlock;
struct UDisksFilesystem;
struct UDisksEncrypted;
struct UDisksDrive;
struct GDBusObjectManager;
struct GVariant;
struct GVariantType;
struct GCancellable;
struct GMainContext;
struct GSource;

/** Public GLib ABI layouts; all UDisks/GObject types remain opaque. */
struct GError {
	Uint32 domain;
	int code;
	char* message;
};

struct GList {
	void* data;
	GList* next;
	GList* prev;
};

/** Optional runtime API, like eefm's Ark D-Bus integration. No UDisks or GLib headers or
 * linker dependencies. The library stays loaded for the lifetime of the process. */
class UDisksFunctions {
  public:
	static const UDisksFunctions& get() {
		static const UDisksFunctions functions;
		return functions;
	}

	UDisksClient* ( *clientNew )( GCancellable*, GError** ){ nullptr };
	GDBusObjectManager* ( *objectManager )( UDisksClient* ){ nullptr };
	GList* ( *objects )( GDBusObjectManager* ){ nullptr };
	UDisksBlock* ( *block )( UDisksObject* ){ nullptr };
	UDisksFilesystem* ( *filesystem )( UDisksObject* ){ nullptr };
	UDisksEncrypted* ( *encrypted )( UDisksObject* ){ nullptr };
	UDisksBlock* ( *cleartext )( UDisksClient*, UDisksBlock* ){ nullptr };
	UDisksDrive* ( *drive )( UDisksClient*, UDisksBlock* ){ nullptr };
	UDisksObject* ( *peekObject )( UDisksClient*, const char* ){ nullptr };
	const char* ( *cryptoBacking )( UDisksBlock* ){ nullptr };
	int ( *system )( UDisksBlock* ){ nullptr };
	int ( *ignore )( UDisksBlock* ){ nullptr };
	const char* const* ( *mountPoints )( UDisksFilesystem* ){ nullptr };
	int ( *ejectable )( UDisksDrive* ){ nullptr };
	int ( *canPowerOff )( UDisksDrive* ){ nullptr };
	int ( *unmount )( UDisksFilesystem*, GVariant*, GCancellable*, GError** ){ nullptr };
	int ( *lock )( UDisksEncrypted*, GVariant*, GCancellable*, GError** ){ nullptr };
	int ( *eject )( UDisksDrive*, GVariant*, GCancellable*, GError** ){ nullptr };
	int ( *powerOff )( UDisksDrive*, GVariant*, GCancellable*, GError** ){ nullptr };
	GVariant* ( *newArray )( const GVariantType*, GVariant* const*, size_t ){ nullptr };
	GVariant* ( *refSink )( GVariant* ){ nullptr };
	void ( *variantUnref )( GVariant* ){ nullptr };
	void ( *objectUnref )( void* ){ nullptr };
	void ( *listFree )( GList*, void ( * )( void* ) ){ nullptr };
	void ( *errorFree )( GError* ){ nullptr };
	GCancellable* ( *cancellableNew )(){ nullptr };
	void ( *cancel )( GCancellable* ){ nullptr };
	GMainContext* ( *contextNew )(){ nullptr };
	void ( *contextPush )( GMainContext* ){ nullptr };
	void ( *contextPop )( GMainContext* ){ nullptr };
	int ( *contextIteration )( GMainContext*, int ){ nullptr };
	void ( *contextUnref )( GMainContext* ){ nullptr };
	GSource* ( *idleNew )(){ nullptr };
	void ( *sourcePriority )( GSource*, int ){ nullptr };
	void ( *sourceCallback )( GSource*, int ( * )( void* ), void*, void ( * )( void* ) ){ nullptr };
	unsigned int ( *sourceAttach )( GSource*, GMainContext* ){ nullptr };
	void ( *sourceUnref )( GSource* ){ nullptr };
	bool loaded{ false };

  private:
	UDisksFunctions() {
		void* handle = Sys::loadObject( "libudisks2.so.0", false );
		loaded = handle && load( handle, clientNew, "udisks_client_new_sync" ) &&
				 load( handle, objectManager, "udisks_client_get_object_manager" ) &&
				 load( handle, objects, "g_dbus_object_manager_get_objects" ) &&
				 load( handle, block, "udisks_object_peek_block" ) &&
				 load( handle, filesystem, "udisks_object_peek_filesystem" ) &&
				 load( handle, encrypted, "udisks_object_peek_encrypted" ) &&
				 load( handle, cleartext, "udisks_client_get_cleartext_block" ) &&
				 load( handle, drive, "udisks_client_get_drive_for_block" ) &&
				 load( handle, peekObject, "udisks_client_peek_object" ) &&
				 load( handle, cryptoBacking, "udisks_block_get_crypto_backing_device" ) &&
				 load( handle, system, "udisks_block_get_hint_system" ) &&
				 load( handle, ignore, "udisks_block_get_hint_ignore" ) &&
				 load( handle, mountPoints, "udisks_filesystem_get_mount_points" ) &&
				 load( handle, ejectable, "udisks_drive_get_ejectable" ) &&
				 load( handle, canPowerOff, "udisks_drive_get_can_power_off" ) &&
				 load( handle, unmount, "udisks_filesystem_call_unmount_sync" ) &&
				 load( handle, lock, "udisks_encrypted_call_lock_sync" ) &&
				 load( handle, eject, "udisks_drive_call_eject_sync" ) &&
				 load( handle, powerOff, "udisks_drive_call_power_off_sync" ) &&
				 load( handle, newArray, "g_variant_new_array" ) &&
				 load( handle, refSink, "g_variant_ref_sink" ) &&
				 load( handle, variantUnref, "g_variant_unref" ) &&
				 load( handle, objectUnref, "g_object_unref" ) &&
				 load( handle, listFree, "g_list_free_full" ) &&
				 load( handle, errorFree, "g_error_free" ) &&
				 load( handle, cancellableNew, "g_cancellable_new" ) &&
				 load( handle, cancel, "g_cancellable_cancel" ) &&
				 load( handle, contextNew, "g_main_context_new" ) &&
				 load( handle, contextPush, "g_main_context_push_thread_default" ) &&
				 load( handle, contextPop, "g_main_context_pop_thread_default" ) &&
				 load( handle, contextIteration, "g_main_context_iteration" ) &&
				 load( handle, contextUnref, "g_main_context_unref" ) &&
				 load( handle, idleNew, "g_idle_source_new" ) &&
				 load( handle, sourcePriority, "g_source_set_priority" ) &&
				 load( handle, sourceCallback, "g_source_set_callback" ) &&
				 load( handle, sourceAttach, "g_source_attach" ) &&
				 load( handle, sourceUnref, "g_source_unref" );
	}

	template <typename Function>
	static bool load( void* handle, Function& function, const char* name ) {
		function = reinterpret_cast<Function>( Sys::loadFunction( handle, name, false ) );
		return function != nullptr;
	}
};

/** A fresh UDisks snapshot, cancellation and GLib context owned by the worker. Subscription
 * cleanup is drained here, never on the application's global/main-thread GLib context. */
class UDisks : NonCopyable {
  public:
	explicit UDisks( const CancellationToken& token ) : mAPI( UDisksFunctions::get() ) {
		if ( !mAPI.loaded )
			return;
		mContext = mAPI.contextNew();
		mAPI.contextPush( mContext );
		mCancellable = mAPI.cancellableNew();
		mCancellation = token.onCancel(
			[cancellable = mCancellable, cancel = mAPI.cancel] { cancel( cancellable ); } );
		mClient = mAPI.clientNew( mCancellable, &mError );
		if ( mClient )
			mObjects = mAPI.objects( mAPI.objectManager( mClient ) );
	}

	~UDisks() {
		// Unregistration waits for a concurrent cancel callback before its borrowed handle dies.
		mCancellation.reset();
		if ( mObjects )
			mAPI.listFree( mObjects, mAPI.objectUnref );
		if ( mClient )
			mAPI.objectUnref( mClient );
		if ( mCancellable )
			mAPI.objectUnref( mCancellable );
		if ( mError )
			mAPI.errorFree( mError );
		if ( mContext ) {
			// GIO queues subscription destroy notifications even after unref returns. Its
			// documented completion barrier is an idle source below G_PRIORITY_DEFAULT.
			// Attach explicitly: context-invoke may run inline while this thread owns it.
			bool drained = false;
			GSource* barrier = mAPI.idleNew();
			mAPI.sourcePriority( barrier, 300 ); // G_PRIORITY_LOW
			mAPI.sourceCallback(
				barrier,
				[]( void* done ) {
					*static_cast<bool*>( done ) = true;
					return 0;
				},
				&drained, nullptr );
			mAPI.sourceAttach( barrier, mContext );
			mAPI.sourceUnref( barrier );
			while ( !drained )
				mAPI.contextIteration( mContext, true );
			mAPI.contextPop( mContext );
			mAPI.contextUnref( mContext );
		}
	}

	void updateCapabilities( std::vector<Place>& devices ) const {
		for ( auto& device : devices ) {
			device.ejectable = false;
			if ( device.section != PlaceSection::Devices || device.uri.getScheme() != "file" )
				continue;
			auto* object = findMount( device.uri.getPath() );
			auto drive = driveFor( object );
			device.ejectable =
				drive && supported( drive.get() ) && !hasProtectedMount( drive.get() );
		}
	}

	std::string eject( const std::string& path ) {
		if ( !mClient )
			return error( "Device ejection requires the UDisks2 runtime library and service." );
		auto drive = driveFor( findMount( path ) );
		if ( !drive || !supported( drive.get() ) || hasProtectedMount( drive.get() ) )
			return "This volume is no longer available or cannot be ejected.";

		// Hold borrowed filesystem proxies through mObjects. Validate the entire drive before
		// unmounting anything: Eject/PowerOff affects all of its partitions.
		SmallVector<RemovalTarget<UDisksFilesystem>, 4> filesystems;
		SmallVector<RemovalTarget<UDisksEncrypted>, 2> encrypted;
		for ( GList* item = mObjects; item; item = item->next ) {
			auto* object = static_cast<UDisksObject*>( item->data );
			auto* fs = mAPI.filesystem( object );
			size_t backingDepth = 0;
			auto otherDrive = driveFor( object, &backingDepth );
			if ( otherDrive.get() != drive.get() )
				continue;
			if ( fs && mounted( fs ) ) {
				size_t mountDepth = 0;
				for ( auto points = mAPI.mountPoints( fs ); points && *points; ++points ) {
					std::string_view path( *points );
					mountDepth = std::max( mountDepth, static_cast<size_t>( std::count(
														   path.begin(), path.end(), '/' ) ) );
				}
				filesystems.push_back( { fs, mountDepth } );
			}
			if ( auto* crypto = mAPI.encrypted( object ) ) {
				if ( auto* cleartext = mAPI.cleartext( mClient, mAPI.block( object ) ) ) {
					mAPI.objectUnref( cleartext );
					encrypted.push_back( { crypto, backingDepth } );
				}
			}
		}
		// Object-manager order is unspecified. Mounted children must precede their parents;
		// encrypted mappings likewise have to close from the innermost layer outwards.
		auto deepestFirst = []( const auto& a, const auto& b ) { return a.depth > b.depth; };
		std::sort( filesystems.begin(), filesystems.end(), deepestFirst );
		std::sort( encrypted.begin(), encrypted.end(), deepestFirst );
		// Sink the options once: generated synchronous calls may consume floating variants.
		// Empty options request a normal unmount, with UDisks/polkit handling authorization.
		auto options = std::unique_ptr<GVariant, decltype( mAPI.variantUnref )>(
			mAPI.refSink(
				mAPI.newArray( reinterpret_cast<const GVariantType*>( "{sv}" ), nullptr, 0 ) ),
			mAPI.variantUnref );
		for ( const auto& target : filesystems ) {
			if ( !mAPI.unmount( target.proxy, options.get(), mCancellable, &mError ) )
				return error( "Unable to unmount the volume." );
		}
		// UDisks refuses to eject/power off a drive with an unlocked encrypted mapping,
		// even after its filesystem was unmounted. Lock only after all unmounts succeeded.
		for ( const auto& target : encrypted ) {
			if ( !mAPI.lock( target.proxy, options.get(), mCancellable, &mError ) )
				return error( "Unable to lock the encrypted volume." );
		}
		auto remove = mAPI.ejectable( drive.get() ) ? mAPI.eject : mAPI.powerOff;
		if ( !remove( drive.get(), options.get(), mCancellable, &mError ) )
			return error( "Unable to eject the drive." );
		return {};
	}

  private:
	template <typename Proxy> struct RemovalTarget {
		Proxy* proxy;
		size_t depth;
	};

	using Drive = std::unique_ptr<UDisksDrive, void ( * )( void* )>;

	Drive driveFor( UDisksObject* object, size_t* depth = nullptr ) const {
		// Unlocked encrypted filesystems belong to their backing drive. Proxies are borrowed
		// from this snapshot; the returned drive has its own reference.
		for ( size_t remaining = 32; object && remaining; --remaining ) {
			auto* block = mAPI.block( object );
			if ( !block )
				break;
			if ( auto* drive = mAPI.drive( mClient, block ) ) {
				if ( depth )
					*depth = 32 - remaining;
				return Drive( drive, mAPI.objectUnref );
			}
			const char* backing = mAPI.cryptoBacking( block );
			if ( !backing || std::string_view( backing ) == "/" )
				break;
			object = mAPI.peekObject( mClient, backing );
		}
		return Drive( nullptr, mAPI.objectUnref );
	}

	bool supported( UDisksDrive* drive ) const {
		return mAPI.ejectable( drive ) || mAPI.canPowerOff( drive );
	}

	bool hasProtectedMount( UDisksDrive* drive ) const {
		for ( GList* item = mObjects; item; item = item->next ) {
			auto* object = static_cast<UDisksObject*>( item->data );
			auto* block = mAPI.block( object );
			auto* fs = mAPI.filesystem( object );
			auto otherDrive = driveFor( object );
			if ( !block || !fs || !mounted( fs ) || otherDrive.get() != drive )
				continue;
			if ( mAPI.system( block ) || mAPI.ignore( block ) )
				return true;
			for ( auto points = mAPI.mountPoints( fs ); points && *points; ++points ) {
				if ( std::string_view( *points ) == "/" )
					return true;
			}
		}
		return false;
	}

	bool mounted( UDisksFilesystem* fs ) const {
		auto points = mAPI.mountPoints( fs );
		return points && *points;
	}

	UDisksObject* findMount( std::string_view path ) const {
		while ( path.size() > 1 && path.back() == '/' )
			path.remove_suffix( 1 );
		for ( GList* item = mObjects; item; item = item->next ) {
			auto* object = static_cast<UDisksObject*>( item->data );
			if ( auto* fs = mAPI.filesystem( object ) ) {
				for ( auto points = mAPI.mountPoints( fs ); points && *points; ++points ) {
					if ( path == *points )
						return object;
				}
			}
		}
		return nullptr;
	}

	std::string error( const char* fallback ) const {
		return mError && mError->message ? mError->message : fallback;
	}

	const UDisksFunctions& mAPI;
	CancellationRegistration mCancellation;
	GMainContext* mContext{ nullptr };
	GCancellable* mCancellable{ nullptr };
	UDisksClient* mClient{ nullptr };
	GList* mObjects{ nullptr };
	GError* mError{ nullptr };
};

} // namespace

DeviceMonitor::DeviceMonitor() : mMountsFd( open( "/proc/self/mounts", O_RDONLY | O_CLOEXEC ) ) {}

DeviceMonitor::~DeviceMonitor() {
	if ( mMountsFd >= 0 )
		close( mMountsFd );
}

bool DeviceMonitor::changed() {
	if ( mMountsFd < 0 )
		return true;
	// procfs consumes the mount-namespace change counter in poll(). This runs on the worker
	// even though this backend can check without waiting or reading the mount table.
	pollfd event{ mMountsFd, POLLPRI, 0 };
	return poll( &event, 1, 0 ) > 0 && ( event.revents & ( POLLPRI | POLLERR ) );
}

void DeviceMonitor::stop() {}

void DeviceMonitor::updateCapabilities( std::vector<Place>& devices,
										const CancellationToken& token ) {
	if ( devices.empty() || token.isCancellationRequested() )
		return;
	UDisks( token ).updateCapabilities( devices );
}

std::string DeviceMonitor::eject( const std::string& path, const CancellationToken& token ) {
	if ( token.isCancellationRequested() )
		return "Device operation cancelled.";
	return UDisks( token ).eject( path );
}

} // namespace eefm

#endif
