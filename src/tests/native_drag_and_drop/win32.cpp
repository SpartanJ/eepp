#include "testexitcode.hpp"
#include <chrono>
#include <eepp/window.hpp>
#include <iostream>
#include <ole2.h>
#include <shellapi.h>
#include <thread>
#include <windows.h>

using namespace EE;
using namespace EE::Window;

namespace {
enum class Scenario : Uint8 {
	Copy,
	Escape,
	Rejection,
	DestroySource,
	CloseSource,
	DestroyCurrentWindow,
	CloseWithDeferredResources,
};

struct TestCase {
	const char* name;
	Scenario scenario;
};

constexpr TestCase testCases[]{
	{ "Copy and pointer re-entry", Scenario::Copy },
	{ "Escape cancellation", Scenario::Escape },
	{ "Rejected drop", Scenario::Rejection },
	{ "Destroy source during OLE", Scenario::DestroySource },
	{ "Close source during OLE", Scenario::CloseSource },
	{ "Destroy current window and keep survivor", Scenario::DestroyCurrentWindow },
	{ "Close source with deferred native resources", Scenario::CloseWithDeferredResources },
};

Uint32 sdlMouseState() {
	// The harness does not link SDL directly. Query the public API of the loaded DLL, with
	// the correct parameter types for each SDL version.
	if ( auto module = GetModuleHandleW( L"SDL3.dll" ) ) {
		auto query = reinterpret_cast<Uint32( __cdecl* )( float*, float* )>(
			GetProcAddress( module, "SDL_GetMouseState" ) );
		return query ? query( nullptr, nullptr ) : EE_BUTTON_LMASK;
	}
	auto query = reinterpret_cast<Uint32( __cdecl* )( int*, int* )>(
		GetProcAddress( GetModuleHandleW( L"SDL2.dll" ), "SDL_GetMouseState" ) );
	return query ? query( nullptr, nullptr ) : EE_BUTTON_LMASK;
}

BOOL CALLBACK findPreviewWindow( HWND window, LPARAM result ) {
	RECT rect{};
	if ( IsWindowVisible( window ) && GetClientRect( window, &rect ) && rect.right == 48 &&
		 rect.bottom == 32 && ( GetWindowLongPtrW( window, GWL_EXSTYLE ) & WS_EX_NOACTIVATE ) ) {
		*reinterpret_cast<HWND*>( result ) = window;
		return FALSE;
	}
	return TRUE;
}

HWND previewWindow() {
	HWND preview = nullptr;
	EnumWindows( findPreviewWindow, reinterpret_cast<LPARAM>( &preview ) );
	return preview;
}

class Target final : public IDropTarget {
  public:
	~Target() {
		if ( retained.tymed )
			ReleaseStgMedium( &retained );
	}

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** result ) override {
		if ( !result )
			return E_POINTER;
		*result = nullptr;
		if ( iid != IID_IUnknown && iid != IID_IDropTarget )
			return E_NOINTERFACE;
		*result = static_cast<IDropTarget*>( this );
		AddRef();
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement( &references ); }

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG count = InterlockedDecrement( &references );
		if ( !count )
			delete this;
		return count;
	}

	HRESULT STDMETHODCALLTYPE DragEnter( IDataObject* data, DWORD, POINTL,
										 DWORD* effect ) override {
		const auto preview = previewWindow();
		previewVisible = preview && IsWindowVisible( preview );
		FORMATETC format{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		*effect =
			!reject && data->QueryGetData( &format ) == S_OK ? DROPEFFECT_COPY : DROPEFFECT_NONE;
		IEnumFORMATETC* formats = nullptr;
		if ( data->EnumFormatEtc( DATADIR_GET, &formats ) == S_OK ) {
			FORMATETC entry{};
			ULONG fetched = 0;
			enumerated = formats->Next( 1, &entry, &fetched ) == S_OK && fetched == 1 &&
						 entry.cfFormat == CF_HDROP;
			formats->Release();
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DragOver( DWORD, POINTL, DWORD* effect ) override {
		*effect = reject ? DROPEFFECT_NONE : DROPEFFECT_COPY;
		if ( destroySource ) {
			destroySource = false;
			const auto handle = source->getWindowHandler();
			Engine::instance()->destroyWindow( source );
			closedImmediately = !Engine::instance()->existsWindow( source ) &&
								!IsWindow( handle ) && !previewWindow();
		} else if ( closeSource ) {
			closeSource = false;
			const auto handle = source->getWindowHandler();
			source->close();
			closedImmediately =
				!source->isOpen() && !previewWindow() &&
				( source->getDeferNativeResourceDestructionOnClose() || !IsWindow( handle ) );
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DragLeave() override { return S_OK; }

	HRESULT STDMETHODCALLTYPE Drop( IDataObject* data, DWORD, POINTL, DWORD* effect ) override {
		FORMATETC format{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM medium{};
		if ( data->GetData( &format, &medium ) != S_OK )
			return E_FAIL;
		// Two independent media must share the immutable handle while retaining its owner.
		if ( data->GetData( &format, &retained ) != S_OK ) {
			ReleaseStgMedium( &medium );
			return E_FAIL;
		}
		sharedMedium =
			medium.hGlobal == retained.hGlobal && medium.pUnkForRelease && retained.pUnkForRelease;
		auto drop = reinterpret_cast<HDROP>( medium.hGlobal );
		UINT count = DragQueryFileW( drop, 0xffffffff, nullptr, 0 );
		valid = count == 2;
		for ( UINT i = 0; i < count; ++i ) {
			wchar_t path[512]{};
			DragQueryFileW( drop, i, path, 512 );
			valid &= std::wstring( path ) ==
					 ( i == 0 ? L"Z:\\tmp\\drag-é-one.txt" : L"Z:\\tmp\\drag-two.txt" );
		}
		ReleaseStgMedium( &medium );
		*effect = DROPEFFECT_COPY;
		++drops;
		return S_OK;
	}

	EE::Window::Window* source{ nullptr };
	LONG references{ 1 };
	int drops{ 0 };
	bool enumerated{ false };
	bool valid{ false };
	bool reject{ false };
	bool destroySource{ false };
	bool closeSource{ false };
	bool closedImmediately{ false };
	bool previewVisible{ false };
	bool sharedMedium{ false };
	STGMEDIUM retained{};
};

void mouseAt( HWND window, DWORD flags ) {
	RECT rect;
	GetClientRect( window, &rect );
	POINT point{ ( rect.right - rect.left ) / 2, ( rect.bottom - rect.top ) / 2 };
	ClientToScreen( window, &point );
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dx = point.x * 65535 / ( GetSystemMetrics( SM_CXSCREEN ) - 1 );
	input.mi.dy = point.y * 65535 / ( GetSystemMetrics( SM_CYSCREEN ) - 1 );
	input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | flags;
	SendInput( 1, &input, sizeof( input ) );
}
} // namespace

int runAutomatedNativeDragTests() {
	if ( FAILED( OleInitialize( nullptr ) ) )
		return 1;
	WNDCLASSW targetClass{};
	targetClass.lpfnWndProc = DefWindowProcW;
	targetClass.hInstance = GetModuleHandleW( nullptr );
	targetClass.lpszClassName = L"EEPPNativeDropTarget";
	targetClass.hbrBackground = reinterpret_cast<HBRUSH>( COLOR_WINDOW + 1 );
	RegisterClassW( &targetClass );
	HWND nativeTarget = CreateWindowExW( 0, L"EEPPNativeDropTarget", L"eepp OLE integration target",
										 WS_OVERLAPPEDWINDOW | WS_VISIBLE, 700, 100, 300, 240,
										 nullptr, nullptr, GetModuleHandleW( nullptr ), nullptr );
	auto* target = new Target;
	if ( !nativeTarget || FAILED( RegisterDragDrop( nativeTarget, target ) ) )
		return 1;
	bool passed = true;
	for ( const auto& testCase : testCases ) {
		const auto scenario = testCase.scenario;
		const bool closesSource = scenario == Scenario::DestroySource ||
								  scenario == Scenario::CloseSource ||
								  scenario == Scenario::DestroyCurrentWindow ||
								  scenario == Scenario::CloseWithDeferredResources;
		auto* survivor = scenario == Scenario::DestroyCurrentWindow
							 ? Engine::instance()->createWindow( { 100, 100, "Surviving window" } )
							 : nullptr;
		int survivorFrames = 0;
		if ( survivor ) {
			survivor->getInput()->pushCallback( [&]( InputEvent* event ) {
				if ( event->Type == InputEvent::EventsSent )
					++survivorFrames;
			} );
		}
		auto* window =
			Engine::instance()->createWindow( { 320, 240, "eepp OLE integration source" } );
		if ( !window || !window->isOpen() )
			return 1;
		window->setPosition( 100, 100 );
		window->show();
		window->raise();
		window->clear();
		window->display();
		Engine::instance()->setCurrentWindow( window );
		if ( scenario == Scenario::CloseWithDeferredResources )
			window->setDeferNativeResourceDestructionOnClose( true );
		auto drag = DragAndDrop::forWindow( *window );
		if ( !drag.getCapabilities().fileSource )
			return TestSkipped;
		HWND origin = window->getWindowHandler();
		SetForegroundWindow( origin );
		bool ordinaryReleasePassed = true;
		if ( scenario == Scenario::Copy ) {
			unsigned releases = 0;
			const auto callback = window->getInput()->pushCallback( [&]( InputEvent* event ) {
				if ( event->Type == InputEvent::MouseButtonUp &&
					 event->button.button == EE_BUTTON_LEFT )
					++releases;
			} );
			mouseAt( origin, MOUSEEVENTF_LEFTDOWN );
			for ( int pump = 0; pump < 20; ++pump ) {
				window->getInput()->update();
				std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
			}
			// Leave SDL's native press intact while clearing eepp's press without a modal drag.
			// Its real button-up must still reach callbacks and produce ordinary click feedback.
			ordinaryReleasePassed &=
				( window->getInput()->getPressTrigger() & EE_BUTTON_LMASK ) != 0;
			window->getInput()->resetButton( EE_BUTTON_LEFT );
			mouseAt( origin, MOUSEEVENTF_LEFTUP );
			bool clicked = false;
			for ( int pump = 0; pump < 20; ++pump ) {
				window->getInput()->update();
				clicked |= ( window->getInput()->getClickTrigger() & EE_BUTTON_LMASK ) != 0;
				std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
			}
			ordinaryReleasePassed &= releases == 1 && clicked;
			window->getInput()->popCallback( callback );
		}
		mouseAt( origin, MOUSEEVENTF_LEFTDOWN );
		for ( int pump = 0; pump < 20; ++pump ) {
			window->getInput()->update();
			std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		}
		SetWindowPos( nativeTarget, HWND_TOPMOST, 700, 100, 300, 240, SWP_SHOWWINDOW );
		target->source = window;
		target->valid = target->enumerated = false;
		target->previewVisible = false;
		target->drops = 0;
		target->reject = scenario == Scenario::Rejection;
		target->destroySource =
			scenario == Scenario::DestroySource || scenario == Scenario::DestroyCurrentWindow;
		target->closeSource =
			scenario == Scenario::CloseSource || scenario == Scenario::CloseWithDeferredResources;
		target->closedImmediately = false;
		int completions = 0;
		bool buttonStateValid = false;
		DragResult result;
		Graphics::Image snapshot( 48, 32, 4, Color::Transparent, false );
		snapshot.fillWithColor( Color( 190, 20, 30, 255 ) );
		DragOffer offer{ { { "Z:\\tmp\\drag-é-one.txt" }, { "Z:\\tmp\\drag-two.txt" } } };
		offer.preview.image = &snapshot;
		offer.preview.hotspot = { 8, 8 };
		auto started = drag.startDrag( offer, [&]( DragResult value ) {
			++completions;
			// Copy/rejection finish after OLE consumes the physical release. Check at
			// completion, before another SDL input update can accidentally repair it.
			buttonStateValid = scenario == Scenario::Escape || closesSource ||
							   !( window->getInput()->getPressTrigger() & EE_BUTTON_LMASK );
			result = std::move( value );
		} );
		if ( started.status != DragStartStatus::Started ) {
			std::cerr << "Start failed: " << started.error << '\n';
			passed = false;
			break;
		}
		const auto preview = previewWindow();
		bool previewPassed =
			preview && IsWindowVisible( preview ) && drag.getCapabilities().dragImage;
		if ( preview ) {
			snapshot.setPixel( 0, 0, Color::Blue );
			const auto dc = GetDC( preview );
			previewPassed &= dc && GetPixel( dc, 0, 0 ) == RGB( 190, 20, 30 );
			if ( dc )
				ReleaseDC( preview, dc );
		}
		std::thread gesture( [=] {
			std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );
			mouseAt( nativeTarget, 0 );
			std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );
			if ( scenario == Scenario::Escape ) {
				INPUT input{};
				input.type = INPUT_KEYBOARD;
				input.ki.wVk = VK_ESCAPE;
				SendInput( 1, &input, sizeof( input ) );
				input.ki.dwFlags = KEYEVENTF_KEYUP;
				SendInput( 1, &input, sizeof( input ) );
			}
			mouseAt( nativeTarget, MOUSEEVENTF_LEFTUP );
		} );
		if ( survivor ) {
			survivorFrames = 0;
			Engine::instance()->updateInput();
		} else {
			window->getInput()->update();
		}
		gesture.join();
		bool reentryPassed = true;
		const bool sdlButtonReleased = !( sdlMouseState() & EE_BUTTON_LMASK );
		if ( scenario == Scenario::Copy ) {
			unsigned clicks = 0;
			const auto callback = window->getInput()->pushCallback( [&]( InputEvent* event ) {
				if ( event->Type == InputEvent::MouseButtonUp &&
					 event->button.button == EE_BUTTON_LEFT )
					++clicks;
			} );
			// Force a late native release while SDL can still hold its modal button state.
			// This goes through SDL's real Win32 window procedure, not Input::pushEvent.
			reentryPassed &= PostMessageW( origin, WM_LBUTTONUP, 0, MAKELPARAM( 20, 20 ) ) != 0;
			SetForegroundWindow( origin );
			mouseAt( origin, 0 );
			for ( int pump = 0; pump < 20; ++pump ) {
				window->getInput()->update();
				reentryPassed &= !( window->getInput()->getClickTrigger() & EE_BUTTON_LMASK );
				std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
			}
			reentryPassed &= !clicks && !( sdlMouseState() & EE_BUTTON_LMASK );
			mouseAt( origin, MOUSEEVENTF_LEFTDOWN );
			for ( int pump = 0; pump < 10; ++pump ) {
				window->getInput()->update();
				std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
			}
			mouseAt( origin, MOUSEEVENTF_LEFTUP );
			for ( int pump = 0; pump < 10; ++pump ) {
				window->getInput()->update();
				std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
			}
			reentryPassed &= clicks == 1;
			window->getInput()->popCallback( callback );
		}
		const bool retainedValid =
			scenario != Scenario::Copy ||
			( target->sharedMedium &&
			  DragQueryFileW( reinterpret_cast<HDROP>( target->retained.hGlobal ), 0xffffffff,
							  nullptr, 0 ) == 2 );
		const bool success =
			completions == 1 && ordinaryReleasePassed && reentryPassed && retainedValid &&
			( !survivor ||
			  ( survivorFrames == 1 && Engine::instance()->getCurrentWindow() == survivor ) ) &&
			buttonStateValid && previewPassed && target->previewVisible && !previewWindow() &&
			( scenario == Scenario::Copy
				  ? result.reason == DragEndReason::Dropped &&
						result.performedAction == DragAction::Copy && target->valid &&
						target->enumerated && target->drops == 1
				  : result.reason == DragEndReason::Cancelled && target->enumerated &&
						target->drops == 0 && ( !closesSource || target->closedImmediately ) );
		std::cout << "Test " << testCase.name << " preview=" << previewPassed
				  << " entered=" << target->previewVisible << " completions=" << completions
				  << " drops=" << target->drops << " buttonState=" << buttonStateValid
				  << " sdlReleased=" << sdlButtonReleased
				  << " ordinaryRelease=" << ordinaryReleasePassed << " reentry=" << reentryPassed
				  << " closedImmediately=" << target->closedImmediately
				  << " survivorFrames=" << survivorFrames << " passed=" << success << '\n';
		passed &= success;
		if ( target->retained.tymed ) {
			ReleaseStgMedium( &target->retained );
			target->retained = {};
		}
		if ( Engine::instance()->existsWindow( window ) )
			Engine::instance()->destroyWindow( window );
		if ( survivor )
			Engine::instance()->destroyWindow( survivor );
	}
	RevokeDragDrop( nativeTarget );
	DestroyWindow( nativeTarget );
	target->Release();
	Engine::destroySingleton();
	OleUninitialize();
	return passed ? 0 : 1;
}
