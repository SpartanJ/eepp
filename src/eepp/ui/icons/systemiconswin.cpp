#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

// Stock icons (SHGetStockIconInfo) and known folders need Windows Vista.
#if !defined( _WIN32_WINNT ) || _WIN32_WINNT < 0x0600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include "systemicons.hpp"

#include <windows.h>

#include <commctrl.h>
#include <commoncontrols.h>
#include <knownfolders.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>

#include <cstring>
#include <vector>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Private {

namespace {

// IID_IImageList: not every MinGW libuuid has it.
const IID ImageListIID = {
	0x46eb5926, 0x582e, 0x4017, { 0x9f, 0xdf, 0xe8, 0x99, 0x8d, 0xaa, 0x09, 0x50 } };

std::wstring toWide( const std::string& text ) {
	if ( text.empty() )
		return {};
	int length = MultiByteToWideChar( CP_UTF8, 0, text.c_str(), -1, nullptr, 0 );
	std::wstring wide( length > 0 ? length - 1 : 0, L'\0' );
	if ( length > 0 )
		MultiByteToWideChar( CP_UTF8, 0, text.c_str(), -1, &wide[0], length );
	return wide;
}

/** Initializes COM for the calling thread for the lifetime of the object, when needed. */
struct ComScope {
	ComScope() {
		HRESULT result = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
		mUninitialize = SUCCEEDED( result );
	}
	~ComScope() {
		if ( mUninitialize )
			CoUninitialize();
	}
	bool mUninitialize{ false };
};

/** RGBA image of an icon; icons without alpha take it from their mask. */
std::unique_ptr<Image> imageFromIcon( HICON icon ) {
	ICONINFO info{};
	if ( !GetIconInfo( icon, &info ) )
		return nullptr;
	std::unique_ptr<Image> image;
	BITMAP bitmap{};
	if ( info.hbmColor && GetObject( info.hbmColor, sizeof( bitmap ), &bitmap ) &&
		 bitmap.bmWidth > 0 && bitmap.bmHeight > 0 ) {
		int width = bitmap.bmWidth;
		int height = bitmap.bmHeight;
		BITMAPINFO bitmapInfo{};
		bitmapInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
		bitmapInfo.bmiHeader.biWidth = width;
		bitmapInfo.bmiHeader.biHeight = -height; // top-down
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;
		std::vector<Uint8> pixels( static_cast<size_t>( width ) * height * 4 );
		HDC dc = GetDC( nullptr );
		if ( GetDIBits( dc, info.hbmColor, 0, height, pixels.data(), &bitmapInfo,
						DIB_RGB_COLORS ) ) {
			bool hasAlpha = false;
			for ( size_t i = 3; i < pixels.size() && !hasAlpha; i += 4 )
				hasAlpha = pixels[i] != 0;
			if ( !hasAlpha && info.hbmMask ) {
				std::vector<Uint8> mask( pixels.size() );
				if ( GetDIBits( dc, info.hbmMask, 0, height, mask.data(), &bitmapInfo,
								DIB_RGB_COLORS ) ) {
					for ( size_t i = 0; i < pixels.size(); i += 4 )
						pixels[i + 3] = mask[i] ? 0 : 255;
				}
			}
			for ( size_t i = 0; i < pixels.size(); i += 4 )
				std::swap( pixels[i], pixels[i + 2] ); // BGRA -> RGBA
			image = std::make_unique<Image>( static_cast<const Uint8*>( pixels.data() ), width,
											 height, 4 );
		}
		ReleaseDC( nullptr, dc );
	}
	if ( info.hbmColor )
		DeleteObject( info.hbmColor );
	if ( info.hbmMask )
		DeleteObject( info.hbmMask );
	return image;
}

/** Image of a system image list entry, from the smallest list that is at least size. */
std::unique_ptr<Image> imageFromSystemIndex( int index, int size ) {
	// The jumbo list pads icons without a 256px variant: 48px is the largest used.
	int list = size <= 16 ? SHIL_SMALL : size <= 32 ? SHIL_LARGE : SHIL_EXTRALARGE;
	IImageList* imageList = nullptr;
	if ( FAILED( SHGetImageList( list, ImageListIID, reinterpret_cast<void**>( &imageList ) ) ) ||
		 !imageList )
		return nullptr;
	HICON icon = nullptr;
	std::unique_ptr<Image> image;
	if ( SUCCEEDED( imageList->GetIcon( index, ILD_TRANSPARENT, &icon ) ) && icon ) {
		image = imageFromIcon( icon );
		DestroyIcon( icon );
	}
	imageList->Release();
	if ( image )
		fitImage( *image, size );
	return image;
}

int stockIndex( SHSTOCKICONID id ) {
	SHSTOCKICONINFO info{};
	info.cbSize = sizeof( info );
	if ( FAILED( SHGetStockIconInfo( id, SHGSI_SYSICONINDEX, &info ) ) )
		return -1;
	return info.iSysImageIndex;
}

int pathIndex( const std::wstring& path, DWORD attributes, UINT flags ) {
	SHFILEINFOW info{};
	if ( !SHGetFileInfoW( path.c_str(), attributes, &info, sizeof( info ),
						  SHGFI_SYSICONINDEX | flags ) )
		return -1;
	return info.iIcon;
}

int knownFolderIndex( REFKNOWNFOLDERID id ) {
	PWSTR path = nullptr;
	int index = -1;
	if ( SUCCEEDED( SHGetKnownFolderPath( id, 0, nullptr, &path ) ) && path )
		index = pathIndex( path, 0, 0 );
	if ( path )
		CoTaskMemFree( path );
	return index;
}

/** The generic folder icon (asked by attributes: the folder does not need to exist). */
int folderIndex() {
	int index = pathIndex( L"folder", FILE_ATTRIBUTE_DIRECTORY, SHGFI_USEFILEATTRIBUTES );
	return index >= 0 ? index : stockIndex( SIID_FOLDER );
}

/** System image index for a freedesktop icon name, or -1. */
int namedIndex( const std::string& name ) {
	if ( name == "folder" )
		return folderIndex();
	if ( name == "user-home" )
		return knownFolderIndex( FOLDERID_Profile );
	if ( name == "user-desktop" )
		return knownFolderIndex( FOLDERID_Desktop );
	if ( name == "folder-documents" )
		return knownFolderIndex( FOLDERID_Documents );
	if ( name == "folder-download" )
		return knownFolderIndex( FOLDERID_Downloads );
	if ( name == "folder-music" )
		return knownFolderIndex( FOLDERID_Music );
	if ( name == "folder-pictures" )
		return knownFolderIndex( FOLDERID_Pictures );
	if ( name == "folder-videos" )
		return knownFolderIndex( FOLDERID_Videos );
	struct Stock {
		const char* name;
		SHSTOCKICONID id;
	};
	static const Stock stock[] = {
		{ "user-trash", SIID_RECYCLER },
		{ "user-trash-full", SIID_RECYCLERFULL },
		{ "drive-harddisk", SIID_DRIVEFIXED },
		{ "drive-harddisk-root", SIID_DRIVEFIXED },
		{ "drive-removable-media", SIID_DRIVEREMOVE },
		{ "drive-removable-media-usb", SIID_DRIVEREMOVE },
		{ "media-optical", SIID_DRIVECD },
		{ "folder-remote", SIID_DRIVENET },
		{ "network-server", SIID_SERVER },
		{ "network-workgroup", SIID_WORLD },
		{ "text-x-generic", SIID_DOCNOASSOC },
		{ "application-x-executable", SIID_APPLICATION },
	};
	for ( const auto& entry : stock )
		if ( name == entry.name )
			return stockIndex( entry.id );
	return -1;
}

} // namespace

std::unique_ptr<Image> loadWindowsIcon( const UISystemIconTheme::Request& request, int size ) {
	ComScope com;
	int index = -1;
	using Kind = UISystemIconTheme::Request::Kind;
	if ( request.kind == Kind::Path )
		index = pathIndex( toWide( request.value ), 0, 0 );
	else if ( request.kind == Kind::File )
		index = request.isDirectory ? folderIndex()
									// By type only: the file does not need to exist.
									: pathIndex( toWide( request.value ), FILE_ATTRIBUTE_NORMAL,
												 SHGFI_USEFILEATTRIBUTES );
	for ( size_t i = 0; index < 0 && i < request.names.size(); ++i )
		index = namedIndex( request.names[i] );
	return index < 0 ? nullptr : imageFromSystemIndex( index, size );
}

}}} // namespace EE::UI::Private

#endif
