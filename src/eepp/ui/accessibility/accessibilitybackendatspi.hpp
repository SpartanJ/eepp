#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKENDATSPI_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKENDATSPI_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD

#include "accessibilitybackend.hpp"
#include "accessibilitydispatchprofile.hpp"
#include <atomic>
#include <cstring>
#include <eepp/core/containers.hpp>
#include <eepp/system/dbus/dbuslibrary.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>

/**
 * AT-SPI (Linux/FreeBSD) accessibility backend.
 *
 * One AtSpiApplication is shared by every window of the process and owns a private connection
 * to the accessibility bus. AT-SPI uses the eepp-dbus module and needs no
 * libdbus development headers. The implementation is split by concern:
 *  - accessibilitybackendatspi.cpp:        connection, I/O thread, dispatch, client tracking and
 *                                          the AccessibilityBackend entry point.
 *  - accessibilitybackendatspievents.cpp:  manager events -> AT-SPI signals, text diffing.
 *  - accessibilitybackendatspitree.cpp:    object paths, roles, states and D-Bus marshalling.
 *  - accessibilitybackendatspimethods.cpp: incoming method calls, one handler per interface.
 */

namespace EE { namespace UI { namespace AtSpi {

using namespace System::Dbus::Native;

inline bool isSelectionContainerRole( AccessibilityRole role ) {
	return role == AccessibilityRole::List || role == AccessibilityRole::Table ||
		   role == AccessibilityRole::Tree || role == AccessibilityRole::TabList;
}

class AtSpiApplication final : public std::enable_shared_from_this<AtSpiApplication> {
  public:
	explicit AtSpiApplication( AccessibilityManager& manager );

	void initialize();

	~AtSpiApplication();

	Uint64 registerManager( AccessibilityManager& manager );

	void unregisterManager( AccessibilityManager& manager );

	bool isAvailable() const {
		return mInitialized.load( std::memory_order_acquire ) && mConnection != nullptr;
	}

	bool isInitialized() const { return mInitialized.load( std::memory_order_acquire ); }

	bool hasActiveClients() const { return mHasActiveClients.load( std::memory_order_acquire ); }

	void update();

	void onEvent( AccessibilityManager& manager, const AccessibilityPendingEvent& event );

	bool onTextChanged( AccessibilityManager& manager, AccessibilityNodeRef ref,
						const AccessibilityTextChange& change );

	void announce( AccessibilityManager& manager, const String& message,
				   AccessibilityLive priority );

  private:
	static constexpr const char* RootPath = "/org/a11y/atspi/accessible/root";
	static constexpr const char* CachePath = "/org/a11y/atspi/cache";
	static constexpr const char* NodePathPrefix = "/org/eepp/a11y/";

	static constexpr AccessibilityNodeRef ApplicationRef{ 0, 1 };

	class ScopedManager {
	  public:
		ScopedManager( AtSpiApplication& application, AccessibilityManager* manager ) :
			mApplication( application ), mPrevious( application.mManager ) {
			application.mManager = manager;
		}

		~ScopedManager() { mApplication.mManager = mPrevious; }

	  private:
		AtSpiApplication& mApplication;
		AccessibilityManager* mPrevious;
	};

	AccessibilityManager* mManager{};
	std::atomic<AccessibilityManager*> mPrimaryManager{};
	String mName;
	std::vector<std::pair<Uint64, AccessibilityManager*>> mManagers;
	std::vector<AccessibilityManager*> mPendingWindowAdds;
	Uint64 mNextManagerId{ 1 };
	const System::Dbus::DbusLibrary* mDBus{ nullptr };
	std::unique_ptr<AccessibilityDispatchProfile> mProfile;
	DBusConnection* mConnection{};
	int mConnectionFd{ -1 };
	int mWakePipe[2]{ -1, -1 };
	std::atomic<bool> mRunning{};
	std::atomic<bool> mDispatchRequested{};
	std::atomic<bool> mHasActiveClients{};
	std::atomic<bool> mInitialized{};
	std::thread mIOThread;
	std::string mBusName;
	std::string mRegistryBusName{ "org.a11y.atspi.Registry" };
	std::string mRegistryPath{ RootPath };
	Int32 mApplicationId{};
	bool mOutgoingPending{};
	/** What a client last saw of a text: its caret and selection, to report their changes, and
	 * its length as of the last change event, to describe a whole-text replacement. Queries never
	 * update the length. No contents are kept: edits arrive as exact change records. */
	struct TextState {
		AccessibilityTextInfo text;
		Int32 length{ -1 };
	};
	UnorderedMap<std::string, TextState> mTextStates;
	/** Sorted selected refs a client last saw, only for queried selection containers. */
	UnorderedMap<std::string, std::vector<AccessibilityNodeRef>> mSelections;
	UnorderedSet<std::string> mClients;
	/** NameHasOwner checks sent when a client was first seen, by reply serial. */
	UnorderedMap<Uint32, std::string> mClientChecks;

	struct PendingText {
		AccessibilityManager* manager;
		AccessibilityNodeRef ref;
		bool wholeText;
	};
	std::vector<PendingText> mPendingTexts;

	// Connection, dispatch and client tracking: accessibilitybackendatspi.cpp

	void initializationFinished() { mInitialized.store( true, std::memory_order_release ); }

	void waitForMessages();

	static DBusHandlerResult handleMessage( DBusConnection*, DBusMessage* message, void* userData );

	void activate( DBusMessage* message );

	/** Watches only this client's disconnection; see the definition for the ordering argument. */
	void watchClient( const char* name );

	void clientGone( const char* name );

	static DBusHandlerResult clientDisconnected( DBusConnection*, DBusMessage* message,
												 void* userData );

	void send( DBusMessage* reply );

	void sendBasic( DBusMessage* request, int type, const void* value );

	void sendError( DBusMessage* request, const char* name, const char* message ) {
		send( mDBus->messageNewError( request, name, message ) );
	}

	void registerApplication();

	// Manager events and text change notifications: accessibilitybackendatspievents.cpp

	/** Starts tracking a text the client is looking at; returns its state. */
	TextState& rememberText( AccessibilityNodeRef ref, const AccessibilityTextInfo& text );

	void queueText( AccessibilityManager& manager, AccessibilityNodeRef ref, bool wholeText );

	void flushPendingTexts();

	void forgetText( AccessibilityManager& manager, AccessibilityNodeRef ref );

	void sendObjectEvent( AccessibilityNodeRef ref, const char* signal, const char* detail,
						  Int32 offset, Int32 length = 0, const String& text = {} );

	void sendTextSelection( AccessibilityNodeRef ref, const AccessibilityTextInfo& text );

	/** Reuses an event-maintained snapshot, initializing it only on the first client query. */
	const std::vector<AccessibilityNodeRef>& rememberSelection( AccessibilityNodeRef ref );

	void sendSelectionChanged( AccessibilityNodeRef ref );

	/** Reports a whole-text replacement: delete everything the client knew, insert the rest. */
	void sendTextReplaced( AccessibilityNodeRef ref );

	/** Reports the replacement of `previous` by `current` as its minimal delete and insert. */
	void sendTextDiff( AccessibilityNodeRef ref, const String& previous, const String& current );

	void sendWindowChanged( AccessibilityManager& manager, bool added, Int32 index );

	// Object paths, tree navigation, roles, states and marshalling:
	// accessibilitybackendatspitree.cpp

	AccessibilityNodeRef refFromPath( const char* path );

	Uint64 currentManagerId() const;

	bool isSceneRoot( AccessibilityNodeRef ref ) const {
		return mManager && ref == mManager->getRoot();
	}

	bool isValid( AccessibilityNodeRef ref ) const {
		return ref == ApplicationRef || ( mManager && mManager->isValid( ref ) );
	}

	AccessibilityNodeInfo getNodeInfo( AccessibilityNodeRef ref, bool includeValue = true ) const;

	/** Scalar roles preserve the application/window projection without reading names or values. */
	AccessibilityRole getNodeRole( AccessibilityNodeRef ref ) const;

	AccessibilityNodeRef getParent( AccessibilityNodeRef ref );

	size_t getChildCount( AccessibilityNodeRef ref );

	AccessibilityNodeRef getChild( AccessibilityNodeRef ref, size_t index );

	std::string pathFromRef( AccessibilityNodeRef ref );

	Uint32 role( AccessibilityRole role ) const;

	const char* roleName( AccessibilityRole role ) const;

	AccessibilityAction actionAt( AccessibilityActions actions, Int32 index ) const;

	AccessibilityActions nativeActions( AccessibilityActions actions ) const;

	const char* actionName( AccessibilityAction action ) const;

	/** The Action key binding ("mnemonic;sequence;accelerator") of the activating action. */
	std::string actionKeyBinding( const AccessibilityNodeInfo& info,
								  AccessibilityAction action ) const;

	void appendBasic( DBusMessageIter& iter, int type, const void* value ) {
		mDBus->messageIterAppendBasic( &iter, type, value );
	}

	bool readPropertySet( DBusMessage* request, const char*& interface, const char*& property,
						  DBusMessageIter& variant );

	void appendRef( DBusMessageIter& iter, AccessibilityNodeRef ref );

	void appendApplicationRef( DBusMessageIter& iter ) { appendRef( iter, ApplicationRef ); }

	void appendEventProperties( DBusMessageIter& iter );

	void appendDesktopRef( DBusMessageIter& iter );

	void appendInterfaces( DBusMessageIter& iter, AccessibilityNodeRef ref,
						   const AccessibilityNodeInfo& info );

	void appendStates( DBusMessageIter& iter, const AccessibilityNodeInfo& info );

	Int32 indexInParent( AccessibilityNodeRef ref );

	Math::Rectf boundsForCoordinateType( AccessibilityNodeRef ref,
										 const AccessibilityNodeInfo& info, Uint32 coordinateType );

	// Incoming method calls, one handler per AT-SPI interface: accessibilitybackendatspimethods.cpp

	DBusHandlerResult handleMessage( DBusMessage* request );

	/** Serves the AT-SPI Cache object. eepp exposes no cached items; clients query lazily. */
	DBusHandlerResult handleCache( DBusMessage* request, const char* interface,
								   const char* member );

	DBusHandlerResult handleAccessible( DBusMessage* request, AccessibilityNodeRef ref,
										const char* member );

	DBusHandlerResult handleApplication( DBusMessage* request, AccessibilityNodeRef ref,
										 const char* member );

	DBusHandlerResult handleComponent( DBusMessage* request, AccessibilityNodeRef ref,
									   const char* member );

	DBusHandlerResult handleAction( DBusMessage* request, AccessibilityNodeRef ref,
									const char* member );

	DBusHandlerResult handleSelection( DBusMessage* request, AccessibilityNodeRef ref,
									   const char* member );

	DBusHandlerResult handleText( DBusMessage* request, AccessibilityNodeRef ref,
								  const char* member );

	DBusHandlerResult handleEditableText( DBusMessage* request, AccessibilityNodeRef ref,
										  const char* member );

	DBusHandlerResult handlePropertiesGet( DBusMessage* request, AccessibilityNodeRef ref );

	DBusHandlerResult handlePropertiesSet( DBusMessage* request, AccessibilityNodeRef ref );
};

}}} // namespace EE::UI::AtSpi

#endif

#endif
