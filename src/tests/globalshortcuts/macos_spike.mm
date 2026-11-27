#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <pthread.h>
#include <string_view>
#include <thread>

namespace {

using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

constexpr EventTypeSpec HotKeyEvent{ kEventClassKeyboard, kEventHotKeyPressed };
constexpr EventHotKeyID HotKeyId{ 0x4545484B, 1 }; // EEHK

enum class Delivery { None, ApplicationThread, WorkerThread };

class Registration {
  public:
	explicit Registration( std::atomic<Delivery>& delivery ) : mDelivery( delivery ) {}

	~Registration() {
		if ( mHotKey )
			UnregisterEventHotKey( mHotKey );
		if ( mHandler )
			RemoveEventHandler( mHandler );
	}

	bool open( EventTargetRef target ) {
		if ( !target ) {
			std::fprintf( stderr, "Carbon returned no event target\n" );
			return false;
		}
		auto status =
			InstallEventHandler( target, onHotKey, 1, &HotKeyEvent, &mDelivery, &mHandler );
		if ( status != noErr ) {
			std::fprintf( stderr, "InstallEventHandler failed (%d)\n", static_cast<int>( status ) );
			return false;
		}
		const auto before = Clock::now();
		status = RegisterEventHotKey( kVK_ANSI_J, cmdKey | controlKey | shiftKey, HotKeyId, target,
									  0, &mHotKey );
		const auto elapsed = std::chrono::duration<double, std::milli>( Clock::now() - before );
		if ( status != noErr ) {
			std::fprintf( stderr, "RegisterEventHotKey failed (%d)\n", static_cast<int>( status ) );
			return false;
		}
		std::printf( "Registered in %.2f ms. Focus another application and press "
					 "Command+Control+Shift+J on the physical J key within 15 seconds.\n",
					 elapsed.count() );
		std::fflush( stdout );
		return true;
	}

  private:
	static OSStatus onHotKey( EventHandlerCallRef, EventRef event, void* context ) {
		EventHotKeyID id{};
		if ( GetEventParameter( event, kEventParamDirectObject, typeEventHotKeyID, nullptr,
								sizeof( id ), nullptr, &id ) != noErr ||
			 id.signature != HotKeyId.signature || id.id != HotKeyId.id )
			return eventNotHandledErr;
		auto& delivery = *static_cast<std::atomic<Delivery>*>( context );
		Delivery first = Delivery::None;
		delivery.compare_exchange_strong( first, pthread_main_np() ? Delivery::ApplicationThread
																   : Delivery::WorkerThread );
		return noErr;
	}

	std::atomic<Delivery>& mDelivery;
	EventHandlerRef mHandler{ nullptr };
	EventHotKeyRef mHotKey{ nullptr };
};

void pumpApplication() {
	@autoreleasepool {
		NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
											untilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]
											   inMode:NSDefaultRunLoopMode
											  dequeue:YES];
		if ( event )
			[NSApp sendEvent:event];
	}
}

bool probe( bool worker ) {
	std::atomic<Delivery> delivery{ Delivery::None };
	if ( !worker ) {
		Registration registration( delivery );
		if ( !registration.open( GetApplicationEventTarget() ) )
			return false;
		const auto deadline = Clock::now() + 15s;
		while ( delivery == Delivery::None && Clock::now() < deadline )
			pumpApplication();
	} else {
		const auto mainQueue = GetCurrentEventQueue();
		const auto mainDispatcher = GetEventDispatcherTarget();
		std::atomic<bool> done{ false };
		std::mutex cleanupMutex;
		std::condition_variable cleanupReady;
		bool canCleanUp = false;
		std::thread thread( [&] {
			@autoreleasepool {
				const auto queue = GetCurrentEventQueue();
				const auto dispatcher = GetEventDispatcherTarget();
				std::printf( "Worker queue is %s from the main queue. The dispatcher is %s.\n",
							 queue != mainQueue ? "distinct" : "indistinguishable",
							 dispatcher == mainDispatcher ? "shared" : "distinct" );
				Registration registration( delivery );
				if ( queue && queue != mainQueue && registration.open( dispatcher ) ) {
					const auto deadline = Clock::now() + 15s;
					while ( delivery == Delivery::None && Clock::now() < deadline ) {
						EventRef event = nullptr;
						const auto status = ReceiveNextEvent( 1, &HotKeyEvent, 0.05, true, &event );
						if ( status == noErr && event ) {
							SendEventToEventTarget( event, dispatcher );
							ReleaseEvent( event );
						} else if ( status != eventLoopTimedOutErr ) {
							std::fprintf( stderr, "ReceiveNextEvent failed (%d)\n",
										  static_cast<int>( status ) );
							break;
						}
					}
				}
				// A shared dispatcher may still be returning from our handler on the main thread.
				done = true;
				std::unique_lock lock( cleanupMutex );
				cleanupReady.wait( lock, [&] { return canCleanUp; } );
			}
		} );
		while ( !done )
			pumpApplication();
		{
			std::lock_guard lock( cleanupMutex );
			canCleanUp = true;
		}
		cleanupReady.notify_one();
		thread.join();
	}

	const auto observed = delivery.load();
	if ( observed == Delivery::None ) {
		std::fprintf( stderr, "No hotkey was delivered\n" );
		return false;
	}
	std::printf( "Hotkey delivered on the %s thread.\n",
				 observed == Delivery::ApplicationThread ? "application" : "worker" );
	return observed == ( worker ? Delivery::WorkerThread : Delivery::ApplicationThread );
}

} // namespace

int main( int argc, char** argv ) {
	const std::string_view mode = argc == 2 ? argv[1] : "";
	if ( mode != "--application" && mode != "--worker" ) {
		std::fprintf( stderr, "Usage: %s --application | --worker\n", argv[0] );
		return 2;
	}
	@autoreleasepool {
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
		[NSApp finishLaunching];
		return probe( mode == "--worker" ) ? 0 : 1;
	}
}
