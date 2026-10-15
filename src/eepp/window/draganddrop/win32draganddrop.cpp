#include "win32draganddrop.hpp"
#include "dragaccess.hpp"
#include "dragpreview.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <algorithm>
#include <climits>
#include <limits>
#include <new>
#include <ole2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windows.h>

namespace EE { namespace Window { namespace Private {

namespace {

class FileDataObject final : public IDataObject {
  public:
	explicit FileDataObject( HGLOBAL files ) : files( files ) {}

	~FileDataObject() { GlobalFree( files ); }

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** result ) override {
		if ( !result )
			return E_POINTER;
		*result = nullptr;
		if ( iid != IID_IUnknown && iid != IID_IDataObject )
			return E_NOINTERFACE;
		*result = static_cast<IDataObject*>( this );
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

	HRESULT STDMETHODCALLTYPE QueryGetData( FORMATETC* format ) override {
		if ( !format )
			return E_POINTER;
		if ( format->cfFormat != CF_HDROP )
			return DV_E_FORMATETC;
		if ( format->dwAspect != DVASPECT_CONTENT )
			return DV_E_DVASPECT;
		if ( format->lindex != -1 )
			return DV_E_LINDEX;
		if ( !( format->tymed & TYMED_HGLOBAL ) )
			return DV_E_TYMED;
		if ( format->ptd )
			return DV_E_DVTARGETDEVICE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetData( FORMATETC* format, STGMEDIUM* medium ) override {
		if ( !medium )
			return E_POINTER;
		*medium = {};
		HRESULT result = QueryGetData( format );
		if ( FAILED( result ) )
			return result;
		medium->tymed = TYMED_HGLOBAL;
		medium->hGlobal = files;
		// The file list is immutable. ReleaseStgMedium releases this owner instead of freeing
		// the shared HGLOBAL, so each receiver can retain it beyond the drag without a copy.
		medium->pUnkForRelease = static_cast<IDataObject*>( this );
		AddRef();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetDataHere( FORMATETC*, STGMEDIUM* ) override { return E_NOTIMPL; }

	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc( FORMATETC*, FORMATETC* output ) override {
		if ( !output )
			return E_POINTER;
		output->ptd = nullptr;
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE SetData( FORMATETC*, STGMEDIUM*, BOOL ) override { return E_NOTIMPL; }

	HRESULT STDMETHODCALLTYPE EnumFormatEtc( DWORD direction, IEnumFORMATETC** result ) override {
		if ( !result )
			return E_POINTER;
		*result = nullptr;
		if ( direction != DATADIR_GET )
			return E_NOTIMPL;
		const FORMATETC format{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		return SHCreateStdEnumFmtEtc( 1, &format, result );
	}

	HRESULT STDMETHODCALLTYPE DAdvise( FORMATETC*, DWORD, IAdviseSink*, DWORD* ) override {
		return OLE_E_ADVISENOTSUPPORTED;
	}

	HRESULT STDMETHODCALLTYPE DUnadvise( DWORD ) override { return OLE_E_ADVISENOTSUPPORTED; }

	HRESULT STDMETHODCALLTYPE EnumDAdvise( IEnumSTATDATA** result ) override {
		if ( result )
			*result = nullptr;
		return OLE_E_ADVISENOTSUPPORTED;
	}

  private:
	HGLOBAL files;
	LONG references{ 1 };
};

class FileDropSource final : public IDropSource {
  public:
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** result ) override {
		if ( !result )
			return E_POINTER;
		*result = nullptr;
		if ( iid != IID_IUnknown && iid != IID_IDropSource )
			return E_NOINTERFACE;
		*result = static_cast<IDropSource*>( this );
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

	HRESULT STDMETHODCALLTYPE QueryContinueDrag( BOOL escape, DWORD keys ) override {
		if ( escape || cancelled ) {
			resetPreview();
			return DRAGDROP_S_CANCEL;
		}
		if ( !( keys & MK_LBUTTON ) ) {
			resetPreview();
			return DRAGDROP_S_DROP;
		}
		preview->updatePosition();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GiveFeedback( DWORD ) override {
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}

	void resetPreview() {
		if ( preview ) {
			preview->reset();
			preview = nullptr;
		}
	}

	NativeDragPreview* preview{ nullptr };
	bool cancelled{ false };

  private:
	LONG references{ 1 };
};

class Win32DragSource : public DragSource {
  public:
	Win32DragSource( NativeWindowContext context, DragCompletionCallback completion ) :
		DragSource( context.owner, std::move( completion ) ),
		window( static_cast<HWND>( context.handles.window ) ) {}

	~Win32DragSource() override {
		if ( dropSource )
			dropSource->resetPreview();
		if ( data )
			data->Release();
		if ( dropSource )
			dropSource->Release();
		if ( oleInitialized )
			OleUninitialize();
	}

	DragStartResult prepare( const DragOffer& offer ) {
		if ( !IsWindow( window ) || !( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) )
			return { "A left-button pointer gesture is required", DragStartStatus::InvalidGesture };
		HRESULT status = OleInitialize( nullptr );
		if ( FAILED( status ) )
			return { "OLE requires an STA on the window thread", DragStartStatus::Unsupported };
		oleInitialized = true;
		size_t characters = 1; // final NUL after the per-path terminators
		const size_t limit =
			( ( std::numeric_limits<SIZE_T>::max )() - sizeof( DROPFILES ) ) / sizeof( wchar_t );
		for ( const auto& file : offer.files ) {
			if ( file.path.size() > INT_MAX )
				return { "File path is too large", DragStartStatus::InvalidOffer };
			const int length = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, file.path.data(),
													file.path.size(), nullptr, 0 );
			if ( !length || static_cast<size_t>( length ) >= limit ||
				 characters > limit - static_cast<size_t>( length ) - 1 )
				return { "Could not convert the file list", DragStartStatus::InvalidOffer };
			characters += static_cast<size_t>( length ) + 1;
		}
		HGLOBAL files = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT,
									 sizeof( DROPFILES ) + characters * sizeof( wchar_t ) );
		if ( !files )
			return { "Could not allocate the file list", DragStartStatus::Failed };
		auto* header = static_cast<DROPFILES*>( GlobalLock( files ) );
		if ( !header ) {
			GlobalFree( files );
			return { "Could not lock the file list", DragStartStatus::Failed };
		}
		header->pFiles = sizeof( DROPFILES );
		header->fWide = TRUE;
		auto* path =
			reinterpret_cast<wchar_t*>( reinterpret_cast<char*>( header ) + sizeof( DROPFILES ) );
		for ( const auto& file : offer.files ) {
			const int length = MultiByteToWideChar(
				CP_UTF8, MB_ERR_INVALID_CHARS, file.path.data(), file.path.size(), path,
				static_cast<int>( std::min<size_t>( characters - 1, INT_MAX ) ) );
			if ( !length ) {
				GlobalUnlock( files );
				GlobalFree( files );
				return { "Could not convert a UTF-8 file path", DragStartStatus::InvalidOffer };
			}
			for ( int i = 0; i < length; ++i ) {
				if ( path[i] == L'/' )
					path[i] = L'\\';
			}
			path += length + 1;
			characters -= length + 1;
		}
		GlobalUnlock( files );
		data = new ( std::nothrow ) FileDataObject( files );
		if ( !data ) {
			GlobalFree( files );
			return { "Could not allocate the OLE data object", DragStartStatus::Failed };
		}
		dropSource = new ( std::nothrow ) FileDropSource;
		if ( !dropSource )
			return { "Could not allocate the OLE drop source", DragStartStatus::Failed };
		dropSource->preview = NativeDragAccess::createNativeDragPreview( *owner, offer, false );
		if ( !dropSource->preview )
			return { "Could not create the drag preview", DragStartStatus::Failed };
		return { {}, DragStartStatus::Started };
	}

	void tick() override {
		if ( completed || modal )
			return;
		if ( !( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) ) {
			cancel();
			return;
		}
		modal = true;
		DWORD effect = DROPEFFECT_NONE;
		HRESULT status = DoDragDrop( data, dropSource, DROPEFFECT_COPY, &effect );
		dropSource->resetPreview();
		// Native callbacks may close or destroy the source synchronously. Invalidation clears
		// owner and the preview before its backend resources disappear; COM owns the file data
		// independently until this modal call returns.
		if ( owner )
			NativeDragAccess::resetNativeDragInput( *owner->getInput() );
		modal = false;
		if ( dropSource->cancelled )
			finish( DragEndReason::Cancelled );
		else if ( status == DRAGDROP_S_DROP && effect == DROPEFFECT_COPY )
			finish( DragEndReason::Dropped, DragAction::Copy );
		else if ( status == DRAGDROP_S_CANCEL || status == DRAGDROP_S_DROP )
			finish( DragEndReason::Cancelled );
		else
			finish( DragEndReason::Failed, DragAction::None, "DoDragDrop failed" );
	}

	bool handleEvent( void* ) override { return false; }

	void cancel() override {
		if ( dropSource ) {
			dropSource->cancelled = true;
			// The popup borrows the source's backend window. Release it even inside OLE so source
			// destruction never leaves a borrowed parent or cursor owner in the native session.
			dropSource->resetPreview();
		}
		if ( !modal )
			finish( DragEndReason::Cancelled );
	}

  private:
	HWND window;
	FileDataObject* data{ nullptr };
	FileDropSource* dropSource{ nullptr };
	bool oleInitialized{ false };
	bool modal{ false };
};
} // namespace

std::unique_ptr<DragSource> Win32DragAndDrop::createSource( NativeWindowContext context,
															const DragOffer& offer,
															DragCompletionCallback completion,
															DragStartResult& result ) {
	auto source = std::make_unique<Win32DragSource>( context, std::move( completion ) );
	result = source->prepare( offer );
	if ( result.status != DragStartStatus::Started )
		return nullptr;
	return source;
}

}}} // namespace EE::Window::Private

#endif
