#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN && defined( EE_SDL_VERSION_2 )

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <ole2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windows.h>
#undef KEY_EXECUTE

#include "utest.hpp"
#include <cstring>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

UTEST( NativeDrag, win32SDL2ReceivesOleFilesAndRevokesTargetOnClose ) {
	WindowSettings settings{ 320, 240, "OLE File Receiver" };
	settings.Style |= WindowStyle::Hidden;
	UIApplication app( settings );
	ASSERT_TRUE( app.getWindow() != nullptr );
	// Keep the application's GL context alive while closing the receiving window.
	auto* window = Engine::instance()->createWindow( settings );
	ASSERT_TRUE( window != nullptr );
	ScopedOp windowCleanup( [] {}, [window] { Engine::instance()->destroyWindow( window ); } );
	HWND native = window->getWindowHandler();
	auto* target = static_cast<IDropTarget*>( GetPropW( native, L"eepp.FileDropTarget" ) );
	ASSERT_TRUE( target != nullptr );
	target->AddRef();
	ScopedOp targetCleanup( [] {}, [target] { target->Release(); } );

	IDataObject* data = nullptr;
	ASSERT_EQ(
		CreateDataCache( nullptr, CLSID_NULL, IID_IDataObject, reinterpret_cast<void**>( &data ) ),
		S_OK );
	ScopedOp dataCleanup( [] {}, [data] { data->Release(); } );
	DWORD effect = DROPEFFECT_COPY;
	EXPECT_EQ( target->DragEnter( data, 0, {}, &effect ), S_OK );
	EXPECT_EQ( effect, static_cast<DWORD>( DROPEFFECT_NONE ) );

	// Use a native COM data object without reading or replacing the user's clipboard.
	FORMATETC format{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	IOleCache* cache = nullptr;
	ASSERT_EQ( data->QueryInterface( IID_IOleCache, reinterpret_cast<void**>( &cache ) ), S_OK );
	ScopedOp cacheCleanup( [] {}, [cache] { cache->Release(); } );
	DWORD connection = 0;
	ASSERT_TRUE( SUCCEEDED( cache->Cache( &format, ADVF_PRIMEFIRST, &connection ) ) );

	constexpr wchar_t paths[] = L"C:\\tmp\\image #画像.png\0C:\\tmp\\other image.png\0";
	STGMEDIUM medium{};
	medium.tymed = TYMED_HGLOBAL;
	medium.hGlobal =
		GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( DROPFILES ) + sizeof( paths ) );
	ASSERT_TRUE( medium.hGlobal != nullptr );
	auto* header = static_cast<DROPFILES*>( GlobalLock( medium.hGlobal ) );
	if ( !header ) {
		GlobalFree( medium.hGlobal );
		ASSERT_TRUE( header != nullptr );
	}
	header->pFiles = sizeof( DROPFILES );
	header->fWide = TRUE;
	std::memcpy( reinterpret_cast<char*>( header ) + sizeof( DROPFILES ), paths, sizeof( paths ) );
	GlobalUnlock( medium.hGlobal );
	const HRESULT stored = data->SetData( &format, &medium, TRUE );
	if ( FAILED( stored ) )
		ReleaseStgMedium( &medium );
	ASSERT_EQ( stored, S_OK );

	effect = DROPEFFECT_MOVE;
	EXPECT_EQ( target->DragEnter( data, 0, {}, &effect ), S_OK );
	EXPECT_EQ( effect, static_cast<DWORD>( DROPEFFECT_NONE ) );
	effect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
	EXPECT_EQ( target->DragOver( 0, {}, &effect ), S_OK );
	EXPECT_EQ( effect, static_cast<DWORD>( DROPEFFECT_COPY ) );
	EXPECT_EQ( target->DragLeave(), S_OK );
	effect = DROPEFFECT_COPY;
	EXPECT_EQ( target->DragOver( 0, {}, &effect ), S_OK );
	EXPECT_EQ( effect, static_cast<DWORD>( DROPEFFECT_NONE ) );

	SmallVector<std::string, 2> received;
	auto* input = window->getInput();
	const auto callback = input->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::FileDropped )
			received.emplace_back( event->file.file );
	} );
	ScopedOp callbackCleanup( [] {}, [input, callback] { input->popCallback( callback ); } );
	for ( int repeat = 0; repeat < 2; ++repeat ) {
		received.clear();
		effect = DROPEFFECT_COPY;
		EXPECT_EQ( target->DragEnter( data, 0, {}, &effect ), S_OK );
		EXPECT_EQ( effect, static_cast<DWORD>( DROPEFFECT_COPY ) );
		EXPECT_EQ( target->Drop( data, 0, { 20, 20 }, &effect ), S_OK );
		EXPECT_EQ( effect, static_cast<DWORD>( DROPEFFECT_COPY ) );
		// OLE only queues SDL events. Application callbacks run during the input update.
		EXPECT_TRUE( received.empty() );
		input->update();
		ASSERT_EQ( received.size(), 2u );
		EXPECT_STDSTREQ( received[0], "C:\\tmp\\image #画像.png" );
		EXPECT_STDSTREQ( received[1], "C:\\tmp\\other image.png" );
	}
	// Repeated drops also prove SDL's DragFinish did not free the source data object's medium.
	STGMEDIUM retained{};
	ASSERT_EQ( data->GetData( &format, &retained ), S_OK );
	EXPECT_EQ( DragQueryFileW( static_cast<HDROP>( retained.hGlobal ), 0xffffffff, nullptr, 0 ),
			   2u );
	ReleaseStgMedium( &retained );
	window->close();
	EXPECT_TRUE( GetPropW( native, L"eepp.FileDropTarget" ) == nullptr );
}

#endif
