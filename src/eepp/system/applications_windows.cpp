#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

// Query Windows 10 association metadata through the existing AssocQueryString API.
// Microsoft's SDK gates these enum values on NTDDI_VERSION; MinGW exposes them unconditionally.
#if !defined( _WIN32_WINNT ) || _WIN32_WINNT < 0x0A00
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#if !defined( NTDDI_VERSION ) || NTDDI_VERSION < 0x0A000000
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif

#include <eepp/system/applications.hpp>

#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>

#include <windows.h>

#include <algorithm>
#include <memory>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <type_traits>
#include <wrl/client.h>

namespace EE { namespace System {

using Microsoft::WRL::ComPtr;

namespace {

struct Apartment {
	HRESULT status{ CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED ) };
	~Apartment() {
		if ( SUCCEEDED( status ) )
			CoUninitialize();
	}
	bool ready() const { return SUCCEEDED( status ) || status == RPC_E_CHANGED_MODE; }
};

struct TaskMemory {
	void operator()( void* pointer ) const { CoTaskMemFree( pointer ); }
};

// Preserve the SDK's __unaligned qualifier and optional strict PIDL types.
using ItemIdList = std::unique_ptr<std::remove_pointer_t<PIDLIST_ABSOLUTE>, TaskMemory>;

std::string takeString( PWSTR text ) {
	std::unique_ptr<wchar_t, TaskMemory> owned( text );
	return text ? String( text ).toUtf8() : std::string{};
}

std::string association( ASSOCSTR field, const std::wstring& extension ) {
	DWORD length = 0;
	AssocQueryStringW( ASSOCF_INIT_IGNOREUNKNOWN, field, extension.c_str(), nullptr, nullptr,
					   &length );
	if ( !length )
		return {};
	std::wstring value( length, L'\0' );
	if ( FAILED( AssocQueryStringW( ASSOCF_INIT_IGNOREUNKNOWN, field, extension.c_str(), nullptr,
									value.data(), &length ) ) )
		return {};
	value.resize( wcslen( value.c_str() ) );
	return String( value ).toUtf8();
}

/** Handlers stay on this COM apartment; menus retain only their owned string descriptions. */
template <typename F> void handlers( const std::wstring& extension, F&& visit ) {
	ComPtr<IEnumAssocHandlers> enumerator;
	if ( FAILED( SHAssocEnumHandlers( extension.c_str(), ASSOC_FILTER_RECOMMENDED, &enumerator ) ) )
		return;
	ComPtr<IAssocHandler> handler;
	while ( enumerator->Next( 1, &handler, nullptr ) == S_OK ) {
		if ( !visit( handler.Get() ) )
			break;
		handler.Reset();
	}
}

std::string handlerName( IAssocHandler* handler ) {
	PWSTR name = nullptr;
	handler->GetName( &name );
	return takeString( name );
}

bool invokeItems( IAssocHandler* handler, PCIDLIST_ABSOLUTE* items, UINT count ) {
	// BHID_DataObject requires a flat selection. Downloads from remote/archive selections
	// have separate temporary parents, so invoke those documents individually.
	bool flat = true;
	if ( count > 1 ) {
		ItemIdList parent( ILCloneFull( items[0] ) );
		if ( !parent || !ILRemoveLastID( parent.get() ) )
			return false;
		for ( UINT i = 1; i < count && flat; ++i )
			flat = ILIsParent( parent.get(), items[i], TRUE );
	}
	if ( flat ) {
		ComPtr<IShellItemArray> selection;
		if ( FAILED( SHCreateShellItemArrayFromIDLists( count, items, &selection ) ) )
			return false;
		ComPtr<IDataObject> data;
		if ( FAILED( selection->BindToHandler( nullptr, BHID_DataObject, IID_PPV_ARGS( &data ) ) ) )
			return false;
		if ( count == 1 )
			return SUCCEEDED( handler->Invoke( data.Get() ) );
		ComPtr<IAssocHandlerInvoker> invoker;
		if ( FAILED( handler->CreateInvoker( data.Get(), &invoker ) ) )
			return false;
		if ( invoker->SupportsSelection() == S_OK )
			return SUCCEEDED( invoker->Invoke() );
	}
	// Single-document handlers can still open a multi-file selection, one document at a time.
	for ( UINT i = 0; i < count; ++i ) {
		if ( !invokeItems( handler, items + i, 1 ) )
			return false;
	}
	return true;
}

bool invoke( IAssocHandler* handler, const std::vector<std::string>& paths ) {
	// Shell items preserve the registered application's invocation protocol (including
	// packaged apps and DDE), rather than guessing an executable's command-line syntax.
	EE::SmallVector<ItemIdList, 8> owned;
	EE::SmallVector<PCIDLIST_ABSOLUTE, 8> items;
	owned.reserve( paths.size() );
	items.reserve( paths.size() );
	for ( const auto& path : paths ) {
		PIDLIST_ABSOLUTE item = nullptr;
		const auto wide = String( path ).toWideString();
		const HRESULT result = SHParseDisplayName( wide.c_str(), nullptr, &item, 0, nullptr );
		owned.emplace_back( item );
		if ( FAILED( result ) )
			return false;
		items.push_back( item );
	}
	return invokeItems( handler, items.data(), static_cast<UINT>( items.size() ) );
}

} // namespace

std::vector<Application> Applications::forFile( const URI& uri, const std::vector<std::string>& ) {
	const auto suffix = FileSystem::fileExtension( uri.getLastPathSegment() );
	if ( suffix.empty() )
		return {};
	Apartment apartment;
	if ( !apartment.ready() )
		return {};
	const auto extension = String( "." + suffix ).toWideString();
	const auto executable = association( ASSOCSTR_EXECUTABLE, extension );
	const auto appId = association( ASSOCSTR_APPID, extension );
	std::vector<Application> out;
	handlers( extension, [&]( IAssocHandler* handler ) {
		auto id = handlerName( handler );
		if ( id.empty() || std::any_of( out.begin(), out.end(), [&]( const Application& app ) {
				 return String::iequals( std::string_view( app.id ).substr( 8 ), id );
			 } ) )
			return true;
		PWSTR name = nullptr;
		handler->GetUIName( &name );
		auto display = takeString( name );
		if ( display.empty() )
			display = FileSystem::fileNameFromPath( id );
		PWSTR resource = nullptr;
		int index = 0;
		handler->GetIconLocation( &resource, &index );
		auto icon = takeString( resource );
		if ( !icon.empty() )
			icon = "\"" + icon + "\"," + std::to_string( index );
		out.push_back( { "handler:" + std::move( id ), std::move( display ), "." + suffix,
						 std::move( icon ), false } );
		return true;
	} );
	auto preferred = std::find_if( out.begin(), out.end(), [&]( const Application& app ) {
		return ( !executable.empty() &&
				 String::iequals( std::string_view( app.id ).substr( 8 ), executable ) ) ||
			   ( !appId.empty() &&
				 String::iequals( std::string_view( app.id ).substr( 8 ), appId ) );
	} );
	if ( preferred != out.end() ) {
		std::rotate( out.begin(), preferred, preferred + 1 );
	} else {
		// A valid default may be absent from the recommended list (also useful on Shell
		// implementations that cannot enumerate handlers). A ProgID retains its open verb.
		auto program = association( ASSOCSTR_PROGID, extension );
		auto name = association( ASSOCSTR_FRIENDLYAPPNAME, extension );
		if ( !program.empty() && !name.empty() ) {
			auto icon = association( ASSOCSTR_APPICONREFERENCE, extension );
			if ( icon.empty() && !executable.empty() )
				icon = "\"" + executable + "\",0";
			out.insert( out.begin(), { "progid:" + std::move( program ), std::move( name ),
									   "." + suffix, std::move( icon ), false } );
		}
	}
	return out;
}

bool Applications::launch( const Application& app, const std::vector<std::string>& paths ) {
	if ( paths.empty() )
		return false;
	Apartment apartment;
	if ( !apartment.ready() )
		return false;
	if ( app.id.starts_with( "progid:" ) ) {
		const auto program = String( app.id.substr( 7 ) ).toWideString();
		for ( const auto& path : paths ) {
			const auto wide = String( path ).toWideString();
			SHELLEXECUTEINFOW launch{};
			launch.cbSize = sizeof( launch );
			// This worker has no message loop; finish the Shell handoff before COM teardown.
			launch.fMask = SEE_MASK_CLASSNAME | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
			launch.lpVerb = L"open";
			launch.lpClass = program.c_str();
			launch.lpFile = wide.c_str();
			launch.nShow = SW_SHOWNORMAL;
			if ( !ShellExecuteExW( &launch ) )
				return false;
		}
		return true;
	}
	if ( !app.id.starts_with( "handler:" ) )
		return false;
	bool opened = false;
	handlers( String( app.exec ).toWideString(), [&]( IAssocHandler* handler ) {
		if ( !String::iequals( handlerName( handler ), std::string_view( app.id ).substr( 8 ) ) )
			return true;
		opened = invoke( handler, paths );
		return false;
	} );
	return opened;
}

}} // namespace EE::System

#endif
