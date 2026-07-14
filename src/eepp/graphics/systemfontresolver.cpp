#ifdef __APPLE__
#define Rect AppleRect
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#undef Rect
#endif

#include <eepp/core/lrucache.hpp>
#include <eepp/core/string.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window/engine.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <dwrite.h>
#include <dwrite_1.h>
#include <windows.h>
#include <wrl/client.h>
#pragma comment( lib, "dwrite.lib" )
#endif

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#if __ANDROID_API__ >= 29
#include <android/font.h>
#include <android/font_matcher.h>
#endif
#include <pugixml/pugixml.hpp>
#endif

#if EE_PLATFORM == EE_PLATFORM_HAIKU
#include <Directory.h>
#include <Entry.h>
#include <Font.h>
#include <Path.h>
#include <String.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include <memory>

using namespace EE::System;

namespace EE::Graphics::SystemFontResolverDetail {

enum class FontProbeKind : Uint8 { Codepoint, SfntTable };

struct FontProbeKey {
	std::string path;
	FT_ULong value;
	FontProbeKind kind;

	bool operator==( const FontProbeKey& other ) const {
		return value == other.value && kind == other.kind && path == other.path;
	}
};

} // namespace EE::Graphics::SystemFontResolverDetail

namespace std {

template <> struct hash<EE::Graphics::SystemFontResolverDetail::FontProbeKey> {
	std::size_t
	operator()( const EE::Graphics::SystemFontResolverDetail::FontProbeKey& key ) const noexcept {
		return hashCombine( std::hash<std::string>{}( key.path ),
							std::hash<FT_ULong>{}( key.value ),
							static_cast<std::size_t>( key.kind ) );
	}
};

} // namespace std

namespace {

using FontProbeKey = EE::Graphics::SystemFontResolverDetail::FontProbeKey;
using FontProbeKind = EE::Graphics::SystemFontResolverDetail::FontProbeKind;

struct FreeTypeState {
	static constexpr std::size_t MaxCachedProbes = 4096;

	FT_Library library{ nullptr };
	Mutex mutex;
	EE::LRUCache<MaxCachedProbes, FontProbeKey, bool> probeCache;

	FreeTypeState() { FT_Init_FreeType( &library ); }

	~FreeTypeState() {
		Lock lock( mutex );
		if ( library )
			FT_Done_FreeType( library );
	}

	bool containsCodepoint( const std::string& path, EE::Uint32 codepoint ) {
		Lock lock( mutex );
		FontProbeKey key{ path, codepoint, FontProbeKind::Codepoint };
		if ( auto cached = probeCache.get( key ) )
			return *cached;

		FT_Face face{ nullptr };
		bool contains = false;
		if ( library && FT_New_Face( library, path.c_str(), 0, &face ) == 0 ) {
			contains = FT_Get_Char_Index( face, codepoint ) != 0;
			FT_Done_Face( face );
		}

		probeCache.put( std::move( key ), contains );
		return contains;
	}

	bool hasSfntTable( const std::string& path, FT_ULong tag ) {
		Lock lock( mutex );
		FontProbeKey key{ path, tag, FontProbeKind::SfntTable };
		if ( auto cached = probeCache.get( key ) )
			return *cached;

		FT_Face face{ nullptr };
		bool found = false;
		if ( library && FT_New_Face( library, path.c_str(), 0, &face ) == 0 ) {
			FT_ULong length = 0;
			found = FT_Load_Sfnt_Table( face, tag, 0, nullptr, &length ) == 0 && length > 0;
			FT_Done_Face( face );
		}

		probeCache.put( std::move( key ), found );
		return found;
	}

	void clearProbeCache() {
		Lock lock( mutex );
		probeCache.clear();
	}
};

std::shared_ptr<FreeTypeState> gFtState;
Mutex gStateInitMutex;

std::shared_ptr<FreeTypeState> getFTState() {
	Lock lock( gStateInitMutex );
	if ( !gFtState )
		gFtState = std::make_shared<FreeTypeState>();
	return gFtState;
}

void destroyFTState() {
	Lock lock( gStateInitMutex );
	gFtState.reset();
}

void clearFTProbeCache() {
	std::shared_ptr<FreeTypeState> state;
	{
		Lock lock( gStateInitMutex );
		state = gFtState;
	}
	if ( state )
		state->clearProbeCache();
}

bool fontHasSfntTable( const std::string& path, FT_ULong tag ) {
	std::shared_ptr<FreeTypeState> state = getFTState();
	return state && state->hasSfntTable( path, tag );
}

bool fontHasSvgTable( const std::string& path ) {
	return fontHasSfntTable( path, FT_MAKE_TAG( 'S', 'V', 'G', ' ' ) );
}

} // namespace

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION( SystemFontResolver )

bool SystemFontResolver::sEnabled = false;

SystemFontResolver::SystemFontResolver() {}

SystemFontResolver::~SystemFontResolver() {
	invalidateCache();
	destroyFTState();
}

void SystemFontResolver::setEnabled( bool enabled ) {
	sEnabled = enabled;
}

bool SystemFontResolver::isEnabled() {
	return sEnabled;
}

void SystemFontResolver::invalidateCache() {
	{
		Lock lock( mMutex );
		mResolveCache.clear();
		mGenericCache.clear();
		mCodepointFallbackCache.clear();
		mFontList.clear();
		mGenericFallbacks.clear();
		mFontListPopulated = false;
	}
	clearFTProbeCache();
}

static std::string normalizeFamily( const std::string& family ) {
	return String::toLower( String::trim( family ) );
}

static const char* genericFamilyStrings[] = { "",		 "serif",	"sans-serif", "monospace",
											  "cursive", "fantasy", "system-ui",  "emoji" };

GenericFamily SystemFontResolver::genericFamilyFromName( const std::string& name ) {
	std::string lower = String::toLower( String::trim( name ) );
	for ( int i = 1; i <= static_cast<int>( GenericFamily::Emoji ); ++i ) {
		if ( lower == genericFamilyStrings[i] )
			return static_cast<GenericFamily>( i );
	}
	return GenericFamily::None;
}

void SystemFontResolver::ensureFontListPopulated() const {
	if ( !mFontListPopulated ) {
		Lock lock( mMutex );
		if ( !mFontListPopulated ) {
			AtomicBoolScopedOp op( mFontListLoading );
			populateFontList();
			populateGenericFallbacks();
			mFontListPopulated = true;
		}
	}
}

std::vector<FontDesc> SystemFontResolver::enumerate() {
	ensureFontListPopulated();
	Lock lock( mMutex );
	return mFontList;
}

std::vector<FontDesc> SystemFontResolver::enumerateFamily( const std::string& family ) {
	ensureFontListPopulated();
	Lock lock( mMutex );
	std::vector<FontDesc> result;
	std::string normFamily = normalizeFamily( family );
	for ( const auto& desc : mFontList ) {
		if ( normalizeFamily( desc.family ) == normFamily )
			result.push_back( desc );
	}
	return result;
}

Uint64 SystemFontResolver::makeCacheKey( const std::string& normFamily, FontWeight weight,
										 FontStretch stretch, bool italic ) {
	return ( static_cast<Uint64>( String::hash( normFamily ) ) << 32 ) |
		   ( static_cast<Uint64>( static_cast<Uint16>( weight ) ) << 16 ) |
		   ( static_cast<Uint64>( static_cast<Uint8>( stretch ) ) << 8 ) |
		   ( static_cast<Uint64>( italic ) );
}

int SystemFontResolver::scoreMatch( const FontQuery& query, const FontDesc& candidate ) {
	int score = 0;
	bool familyMatched = false;

	std::string qFamily = normalizeFamily( query.family );
	std::string cFamily = normalizeFamily( candidate.family );

	if ( cFamily.empty() )
		return -1;

	if ( qFamily.empty() )
		return -1;

	if ( cFamily == qFamily ) {
		score -= 100;
		familyMatched = true;
	} else if ( String::startsWith( cFamily, qFamily ) ) {
		score -= 60;
		familyMatched = true;
	} else if ( String::startsWith( qFamily, cFamily ) ) {
		score -= 50;
		familyMatched = true;
	} else {
		score += 200;
	}

	if ( candidate.monospace && qFamily == "monospace" )
		score -= 50;

	int weightDiff =
		eeabs( static_cast<int>( candidate.weight ) - static_cast<int>( query.weight ) );
	score += weightDiff / 100;

	int stretchDiff =
		eeabs( static_cast<int>( candidate.stretch ) - static_cast<int>( query.stretch ) );
	score += stretchDiff * 10;

	if ( candidate.italic == query.italic )
		score -= 30;
	else
		score += 40;

	return familyMatched ? score : -1;
}

FontDesc SystemFontResolver::resolve( const FontQuery& query ) {
	if ( query.family.empty() )
		return FontDesc();

	ensureFontListPopulated();

	std::string normFamily = normalizeFamily( query.family );

	Lock lock( mMutex );

	Uint64 key = makeCacheKey( normFamily, query.weight, query.stretch, query.italic );

	auto cacheIt = mResolveCache.find( key );
	if ( cacheIt != mResolveCache.end() )
		return cacheIt->second;

	GenericFamily generic = genericFamilyFromName( query.family );
	if ( generic != GenericFamily::None ) {
		FontDesc result = resolveGeneric( generic, query.weight, query.italic );
		mResolveCache[key] = result;
		return result;
	}

	FontDesc best;
	int bestScore = 100000;

	for ( const auto& desc : mFontList ) {
		int score = scoreMatch( query, desc );
		if ( score != -1 && score < bestScore ) {
			bestScore = score;
			best = desc;
		}
	}

	if ( bestScore == 100000 )
		mResolveCache[key] = FontDesc();
	else
		mResolveCache[key] = best;
	return mResolveCache[key];
}

FontDesc SystemFontResolver::resolveFromNamesList( const std::string& namesList, FontWeight weight,
												   bool italic ) {
	ensureFontListPopulated();

	FontDesc result;
	String::readBySeparatorStoppable(
		namesList,
		[&result, weight, italic]( std::string_view name ) {
			name = String::trim( name, ' ' );
			name = String::trim( name, '\'' );
			name = String::trim( name, '"' );
			FontQuery query;
			query.family = std::string{ name };
			query.weight = weight;
			query.italic = italic;
			result = SystemFontResolver::instance()->resolve( query );
			return !result.path.empty();
		},
		',' );

	return result;
}

FontDesc SystemFontResolver::resolveGeneric( GenericFamily generic, FontWeight weight,
											 bool italic ) {
	ensureFontListPopulated();

	Lock lock( mMutex );

	Uint32 cacheKey = ( static_cast<Uint32>( generic ) << 16 ) |
					  ( static_cast<Uint32>( weight ) << 1 ) | ( italic ? 1 : 0 );

	auto it = mGenericCache.find( cacheKey );
	if ( it != mGenericCache.end() )
		return it->second;

	FontDesc result;

	for ( const auto& entry : mGenericFallbacks ) {
		if ( entry.generic == generic ) {
			int entryWeight = static_cast<int>( entry.desc.weight );
			int queryWeight = static_cast<int>( weight );
			int weightDiff = eeabs( entryWeight - queryWeight );
			bool styleMatch = entry.desc.italic == italic;
			int bestWeightDiff =
				result.path.empty()
					? 100000
					: eeabs( static_cast<int>( result.weight ) - static_cast<int>( weight ) );
			bool bestStyleMatch = result.path.empty() ? false : ( result.italic == italic );

			if ( styleMatch && !bestStyleMatch ) {
				result = entry.desc;
			} else if ( styleMatch == bestStyleMatch &&
						( result.path.empty() || weightDiff < bestWeightDiff ) ) {
				result = entry.desc;
			}
		}
	}

	mGenericCache[cacheKey] = result;
	return result;
}

FontDesc SystemFontResolver::getSystemFont() const {
	ensureFontListPopulated();
	Lock lock( mMutex );
	if ( !mGenericFallbacks.empty() ) {
		for ( const auto& entry : mGenericFallbacks ) {
			if ( entry.generic == GenericFamily::SystemUi )
				return entry.desc;
		}
	}
	return FontDesc();
}

FontDesc SystemFontResolver::getSystemMonospaceFont() const {
	ensureFontListPopulated();
	Lock lock( mMutex );
	if ( !mGenericFallbacks.empty() ) {
		for ( const auto& entry : mGenericFallbacks ) {
			if ( entry.generic == GenericFamily::Monospace )
				return entry.desc;
		}
	}
	return FontDesc();
}

FontDesc SystemFontResolver::getFallbackForCodepoint( Uint32 codepoint, FontWeight weight,
													  bool italic ) {
	ensureFontListPopulated();
	const bool isEmoji = Font::isEmojiCodePoint( codepoint );

	std::vector<FontDesc> snapshot;
	{
		Lock lock( mMutex );

		Uint32 cacheKey = codepoint;
		auto it = mCodepointFallbackCache.find( cacheKey );
		if ( it != mCodepointFallbackCache.end() ) {
			const std::string& path = it->second;
			if ( path.empty() )
				return FontDesc();
			for ( const auto& desc : mFontList ) {
				if ( desc.path == path ) {
					if ( isEmoji && fontHasSvgTable( desc.path ) )
						break;
					FontDesc result = desc;
					result.weight = weight;
					result.italic = italic;
					return result;
				}
			}
		}

		snapshot = mFontList;
	}

	for ( const auto& desc : snapshot ) {
		if ( fontContainsCodepoint( desc.path, codepoint ) ) {
			if ( isEmoji && fontHasSvgTable( desc.path ) )
				continue;

			Lock lock( mMutex );
			mCodepointFallbackCache[codepoint] = desc.path;
			FontDesc result = desc;
			result.weight = weight;
			result.italic = italic;
			return result;
		}
	}

	{
		Lock lock( mMutex );
		mCodepointFallbackCache[codepoint] = "";
	}
	return FontDesc();
}

bool SystemFontResolver::fontContainsCodepoint( const std::string& path, Uint32 codepoint ) {
	std::shared_ptr<FreeTypeState> state = getFTState();
	return state && state->containsCodepoint( path, codepoint );
}

void SystemFontResolver::populateGenericFallbacks() const {
	mGenericFallbacks.clear();

	struct Mapping {
		GenericFamily generic;
		const char* family;
	};

	static const Mapping mappings[] = {
		// Prefer native/open desktop families for CSS generics.
		// Exact requests such as "Arial" or "Verdana" are still handled by resolve().
		{ GenericFamily::Serif, "noto serif" },
		{ GenericFamily::Serif, "dejavu serif" },
		{ GenericFamily::Serif, "liberation serif" },
		{ GenericFamily::Serif, "times new roman" },
		{ GenericFamily::Serif, "times" },
		{ GenericFamily::Serif, "serif" },

		{ GenericFamily::SansSerif, "noto sans" },
		{ GenericFamily::SansSerif, "dejavu sans" },
		{ GenericFamily::SansSerif, "liberation sans" },
		{ GenericFamily::SansSerif, "roboto" },
		{ GenericFamily::SansSerif, "arial" },
		{ GenericFamily::SansSerif, "helvetica" },
		{ GenericFamily::SansSerif, "sans-serif" },

		{ GenericFamily::Monospace, "noto sans mono" },
		{ GenericFamily::Monospace, "dejavu sans mono" },
		{ GenericFamily::Monospace, "liberation mono" },
		{ GenericFamily::Monospace, "droid sans mono" },
		{ GenericFamily::Monospace, "consolas" },
		{ GenericFamily::Monospace, "menlo" },
		{ GenericFamily::Monospace, "monospace" },
		{ GenericFamily::Cursive, "comic sans ms" },
		{ GenericFamily::Cursive, "apple chancery" },
		{ GenericFamily::Cursive, "cursive" },
		{ GenericFamily::Fantasy, "impact" },
		{ GenericFamily::Fantasy, "papyrus" },
		{ GenericFamily::Fantasy, "fantasy" },
		{ GenericFamily::SystemUi, "segoe ui" },
		{ GenericFamily::SystemUi, "sf pro" },
		{ GenericFamily::SystemUi, "dejavu sans" },
		{ GenericFamily::SystemUi, "system-ui" },
		{ GenericFamily::Emoji, "segoe ui emoji" },
		{ GenericFamily::Emoji, "apple color emoji" },
		{ GenericFamily::Emoji, "noto color emoji" },
		{ GenericFamily::Emoji, "emoji" },
	};

	for ( const auto& mapping : mappings ) {
		std::string searchFamily = normalizeFamily( mapping.family );
		for ( const auto& desc : mFontList ) {
			std::string descFamily = normalizeFamily( desc.family );
			if ( descFamily == searchFamily ) {
				mGenericFallbacks.push_back( { mapping.generic, desc } );
			}
		}
	}
}

// =====================================================================
// Platform: Windows (DirectWrite)
// =====================================================================
#if EE_PLATFORM == EE_PLATFORM_WIN

static std::string wideToUtf8( const WCHAR* wstr ) {
	if ( !wstr || !*wstr )
		return {};
	int len = WideCharToMultiByte( CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr );
	if ( len <= 0 )
		return {};
	std::string result( len - 1, '\0' );
	WideCharToMultiByte( CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr );
	return result;
}

static IDWriteFactory* getDWriteFactory() {
	static IDWriteFactory* sFactory = nullptr;
	if ( !sFactory ) {
		HRESULT hr = DWriteCreateFactory( DWRITE_FACTORY_TYPE_SHARED, __uuidof( IDWriteFactory ),
										  reinterpret_cast<IUnknown**>( &sFactory ) );
		if ( FAILED( hr ) )
			return nullptr;
	}
	return sFactory;
}

void SystemFontResolver::populateFontList() const {
	using Microsoft::WRL::ComPtr;

	IDWriteFactory* factory = getDWriteFactory();
	if ( !factory )
		return;

	ComPtr<IDWriteFontCollection> collection;
	HRESULT hr = factory->GetSystemFontCollection( &collection, FALSE );
	if ( FAILED( hr ) || !collection )
		return;

	UINT32 familyCount = collection->GetFontFamilyCount();

	for ( UINT32 i = 0; i < familyCount; ++i ) {
		ComPtr<IDWriteFontFamily> fontFamily;
		hr = collection->GetFontFamily( i, &fontFamily );
		if ( FAILED( hr ) || !fontFamily )
			continue;

		ComPtr<IDWriteLocalizedStrings> familyNames;
		hr = fontFamily->GetFamilyNames( &familyNames );
		if ( FAILED( hr ) || !familyNames )
			continue;

		UINT32 nameLen = 0;
		hr = familyNames->GetStringLength( 0, &nameLen );
		if ( FAILED( hr ) )
			continue;

		std::wstring familyNameW( nameLen + 1, L'\0' );
		familyNames->GetString( 0, &familyNameW[0], nameLen + 1 );

		std::string familyName = wideToUtf8( familyNameW.c_str() );
		if ( familyName.empty() )
			continue;

		UINT32 fontCount = fontFamily->GetFontCount();
		for ( UINT32 j = 0; j < fontCount; ++j ) {
			ComPtr<IDWriteFont> dwriteFont;
			hr = fontFamily->GetFont( j, &dwriteFont );
			if ( FAILED( hr ) || !dwriteFont )
				continue;

			if ( dwriteFont->IsSymbolFont() )
				continue;

			DWRITE_FONT_WEIGHT dwWeight = dwriteFont->GetWeight();
			DWRITE_FONT_STRETCH dwStretch = dwriteFont->GetStretch();
			DWRITE_FONT_STYLE dwStyle = dwriteFont->GetStyle();

			ComPtr<IDWriteFontFace> fontFace;
			hr = dwriteFont->CreateFontFace( &fontFace );
			if ( FAILED( hr ) || !fontFace )
				continue;

			UINT32 fileCount = 0;
			hr = fontFace->GetFiles( &fileCount, nullptr );
			if ( FAILED( hr ) || fileCount == 0 )
				continue;

			// Get raw pointers temporarily, then immediately take ownership
			std::vector<IDWriteFontFile*> rawFiles( fileCount, nullptr );
			hr = fontFace->GetFiles( &fileCount, rawFiles.data() );
			if ( FAILED( hr ) )
				continue;

			for ( UINT32 k = 0; k < fileCount; ++k ) {
				ComPtr<IDWriteFontFile> fontFile;
				fontFile.Attach( rawFiles[k] ); // Attach takes ownership and guarantees Release()

				ComPtr<IDWriteFontFileLoader> loader;
				hr = fontFile->GetLoader( &loader );
				if ( FAILED( hr ) || !loader )
					continue;

				ComPtr<IDWriteLocalFontFileLoader> localLoader;
				hr = loader.As( &localLoader ); // .As() replaces QueryInterface boilerplate
				if ( FAILED( hr ) || !localLoader )
					continue;

				const void* refKey = nullptr;
				UINT32 refKeySize = 0;
				hr = fontFile->GetReferenceKey( &refKey, &refKeySize );
				if ( FAILED( hr ) )
					continue;

				UINT32 pathLen = 0;
				hr = localLoader->GetFilePathLengthFromKey( refKey, refKeySize, &pathLen );
				if ( FAILED( hr ) )
					continue;

				std::wstring filePathW( pathLen + 1, L'\0' );
				hr = localLoader->GetFilePathFromKey( refKey, refKeySize, &filePathW[0],
													  pathLen + 1 );
				if ( FAILED( hr ) )
					continue;

				std::string filePath = wideToUtf8( filePathW.c_str() );
				if ( filePath.empty() )
					continue;

				FontDesc desc;
				desc.family = familyName;
				desc.path = filePath;
				desc.faceIndex = fontFace->GetIndex();
				desc.weight = static_cast<FontWeight>( static_cast<Uint16>( dwWeight ) );
				desc.stretch = static_cast<FontStretch>( static_cast<Uint8>( dwStretch ) );
				desc.italic =
					( dwStyle == DWRITE_FONT_STYLE_ITALIC || dwStyle == DWRITE_FONT_STYLE_OBLIQUE );
				desc.monospace = false;

#ifdef _MSC_VER
				__if_exists( IDWriteFont1 ) {
#else
				{
#endif
					ComPtr<IDWriteFont1> dwriteFont1;
					if ( SUCCEEDED( dwriteFont.As( &dwriteFont1 ) ) && dwriteFont1 ) {
						DWRITE_PANOSE panose;
						dwriteFont1->GetPanose( &panose );
						const BYTE* raw = reinterpret_cast<const BYTE*>( &panose );
						desc.monospace = ( raw[0] == 2 && raw[3] == 9 );
					}
				}

				mFontList.push_back( desc );
			}
		}
	}
}

// =====================================================================
// Platform: macOS / iOS (Core Text)
// =====================================================================
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS

static std::string cfStringToStd( CFStringRef str ) {
	if ( !str )
		return {};
	CFIndex len = CFStringGetLength( str );
	CFIndex maxLen = CFStringGetMaximumSizeForEncoding( len, kCFStringEncodingUTF8 );
	std::string result( maxLen, '\0' );
	CFStringGetCString( str, &result[0], maxLen, kCFStringEncodingUTF8 );
	result.resize( strlen( result.c_str() ) );
	return result;
}

static FontWeight ctWeightToFontWeight( CGFloat weight ) {
	if ( weight <= -0.8 )
		return FontWeight::Thin;
	if ( weight <= -0.6 )
		return FontWeight::ExtraLight;
	if ( weight <= -0.4 )
		return FontWeight::Light;
	if ( weight <= 0.0 )
		return FontWeight::Normal;
	if ( weight <= 0.2 )
		return FontWeight::Medium;
	if ( weight <= 0.3 )
		return FontWeight::SemiBold;
	if ( weight <= 0.4 )
		return FontWeight::Bold;
	if ( weight <= 0.6 )
		return FontWeight::ExtraBold;
	return FontWeight::Black;
}

static FontStretch ctWidthToFontStretch( CGFloat width ) {
	if ( width <= -0.8 )
		return FontStretch::UltraCondensed;
	if ( width <= -0.6 )
		return FontStretch::ExtraCondensed;
	if ( width <= -0.4 )
		return FontStretch::Condensed;
	if ( width <= -0.2 )
		return FontStretch::SemiCondensed;
	if ( width <= 0.1 )
		return FontStretch::Normal;
	if ( width <= 0.4 )
		return FontStretch::SemiExpanded;
	if ( width <= 0.6 )
		return FontStretch::Expanded;
	if ( width <= 0.8 )
		return FontStretch::ExtraExpanded;
	return FontStretch::UltraExpanded;
}

void SystemFontResolver::populateFontList() const {
	CFArrayRef descriptors = CTFontManagerCopyAvailableFontFamilyNames();
	if ( !descriptors )
		return;

	CFIndex count = CFArrayGetCount( descriptors );

	for ( CFIndex i = 0; i < count; ++i ) {
		CFStringRef familyNameRef = (CFStringRef)CFArrayGetValueAtIndex( descriptors, i );
		std::string familyName = cfStringToStd( familyNameRef );
		if ( familyName.empty() )
			continue;

		CFMutableDictionaryRef queryDict =
			CFDictionaryCreateMutable( kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks,
									   &kCFTypeDictionaryValueCallBacks );
		CFDictionaryAddValue( queryDict, kCTFontFamilyNameAttribute, familyNameRef );

		CTFontDescriptorRef familyDesc = CTFontDescriptorCreateWithAttributes( queryDict );
		CFRelease( queryDict );

		CFSetRef mandatoryAttrs = CFSetCreate(
			kCFAllocatorDefault, (const void**)&kCTFontURLAttribute, 1, &kCFTypeSetCallBacks );

		CFArrayRef matchingDescs =
			CTFontDescriptorCreateMatchingFontDescriptors( familyDesc, mandatoryAttrs );
		CFRelease( mandatoryAttrs );
		CFRelease( familyDesc );

		if ( !matchingDescs )
			continue;

		CFIndex matchCount = CFArrayGetCount( matchingDescs );

		for ( CFIndex j = 0; j < matchCount; ++j ) {
			CTFontDescriptorRef desc =
				(CTFontDescriptorRef)CFArrayGetValueAtIndex( matchingDescs, j );

			CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute( desc, kCTFontURLAttribute );
			if ( !url )
				continue;

			CFStringRef pathRef = CFURLCopyFileSystemPath( url, kCFURLPOSIXPathStyle );
			CFRelease( url );

			std::string fontPath = cfStringToStd( pathRef );
			CFRelease( pathRef );

			if ( fontPath.empty() )
				continue;

			CFDictionaryRef traits =
				(CFDictionaryRef)CTFontDescriptorCopyAttribute( desc, kCTFontTraitsAttribute );
			CGFloat weightVal = 0.0;
			CGFloat widthVal = 0.0;
			CGFloat slantVal = 0.0;
			int isMono = 0;

			if ( traits ) {
				CFNumberRef weightNum =
					(CFNumberRef)CFDictionaryGetValue( traits, kCTFontWeightTrait );
				if ( weightNum )
					CFNumberGetValue( weightNum, kCFNumberCGFloatType, &weightVal );

				CFNumberRef widthNum =
					(CFNumberRef)CFDictionaryGetValue( traits, kCTFontWidthTrait );
				if ( widthNum )
					CFNumberGetValue( widthNum, kCFNumberCGFloatType, &widthVal );

				CFNumberRef slantNum =
					(CFNumberRef)CFDictionaryGetValue( traits, kCTFontSlantTrait );
				if ( slantNum )
					CFNumberGetValue( slantNum, kCFNumberCGFloatType, &slantVal );

				CFNumberRef symbolicTraitsNum =
					(CFNumberRef)CFDictionaryGetValue( traits, kCTFontSymbolicTrait );
				if ( symbolicTraitsNum ) {
					int symbolicTraits = 0;
					CFNumberGetValue( symbolicTraitsNum, kCFNumberIntType, &symbolicTraits );
					isMono = ( symbolicTraits & kCTFontMonoSpaceTrait ) != 0;
				}

				CFRelease( traits );
			}

			FontDesc fontDesc;
			fontDesc.family = familyName;
			fontDesc.path = fontPath;
			fontDesc.faceIndex = static_cast<Uint32>( j );
			fontDesc.weight = ctWeightToFontWeight( weightVal );
			fontDesc.stretch = ctWidthToFontStretch( widthVal );
			fontDesc.italic = ( slantVal > 0.0 );
			fontDesc.monospace = ( isMono != 0 );

			mFontList.push_back( fontDesc );
		}

		CFRelease( matchingDescs );
	}

	CFRelease( descriptors );
}

// =====================================================================
// Platform: Linux / FreeBSD (Fontconfig — dynamically loaded)
// =====================================================================
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD

struct FcLib {
	void* handle{ nullptr };

	using FcChar8 = unsigned char;
	using FcBool = int;
	using FcResult = int;

	struct FcConfig;
	struct FcPattern;
	struct FcObjectSet;
	struct FcFontSet {
		int nfont;
		int sfont;
		FcPattern** fonts;
	};

	static constexpr int FcResultMatch = 0;

	static constexpr int FC_WEIGHT_THIN = 0;
	static constexpr int FC_WEIGHT_EXTRALIGHT = 40;
	static constexpr int FC_WEIGHT_LIGHT = 50;
	static constexpr int FC_WEIGHT_REGULAR = 80;
	static constexpr int FC_WEIGHT_MEDIUM = 100;
	static constexpr int FC_WEIGHT_SEMIBOLD = 180;
	static constexpr int FC_WEIGHT_BOLD = 200;
	static constexpr int FC_WEIGHT_EXTRABOLD = 205;
	static constexpr int FC_WEIGHT_BLACK = 210;

	static constexpr int FC_WIDTH_ULTRACONDENSED = 50;
	static constexpr int FC_WIDTH_EXTRACONDENSED = 63;
	static constexpr int FC_WIDTH_CONDENSED = 75;
	static constexpr int FC_WIDTH_SEMICONDENSED = 87;
	static constexpr int FC_WIDTH_NORMAL = 100;
	static constexpr int FC_WIDTH_SEMIEXPANDED = 113;
	static constexpr int FC_WIDTH_EXPANDED = 125;
	static constexpr int FC_WIDTH_EXTRAEXPANDED = 150;

	static constexpr int FC_SLANT_ROMAN = 0;
	static constexpr int FC_SLANT_ITALIC = 100;
	static constexpr int FC_SLANT_OBLIQUE = 110;

	static constexpr int FC_PROPORTIONAL = 0;
	static constexpr int FC_MONO = 100;

	FcConfig* ( *InitLoadConfigAndFonts )( void );
	void ( *ConfigDestroy )( FcConfig* );
	void ( *Fini )( void );
	FcPattern* ( *PatternCreate )( void );
	FcObjectSet* ( *ObjectSetCreate )( void );
	FcBool ( *ObjectSetAdd )( FcObjectSet*, const char* );
	FcFontSet* ( *FontList )( FcConfig*, FcPattern*, FcObjectSet* );
	void ( *PatternDestroy )( FcPattern* );
	void ( *ObjectSetDestroy )( FcObjectSet* );
	void ( *FontSetDestroy )( FcFontSet* );
	FcResult ( *PatternGetString )( const FcPattern*, const char*, int, FcChar8** );
	FcResult ( *PatternGetInteger )( const FcPattern*, const char*, int, int* );

	bool load() {
		handle = Sys::loadObject( "libfontconfig.so.1" );
		if ( !handle )
			handle = Sys::loadObject( "libfontconfig.so" );
		if ( !handle )
			return false;

		InitLoadConfigAndFonts = (decltype( InitLoadConfigAndFonts ))Sys::loadFunction(
			handle, "FcInitLoadConfigAndFonts" );
		ConfigDestroy = (decltype( ConfigDestroy ))Sys::loadFunction( handle, "FcConfigDestroy" );
		Fini = (decltype( Fini ))Sys::loadFunction( handle, "FcFini" );
		PatternCreate = (decltype( PatternCreate ))Sys::loadFunction( handle, "FcPatternCreate" );
		ObjectSetCreate =
			(decltype( ObjectSetCreate ))Sys::loadFunction( handle, "FcObjectSetCreate" );
		ObjectSetAdd = (decltype( ObjectSetAdd ))Sys::loadFunction( handle, "FcObjectSetAdd" );
		FontList = (decltype( FontList ))Sys::loadFunction( handle, "FcFontList" );
		PatternDestroy =
			(decltype( PatternDestroy ))Sys::loadFunction( handle, "FcPatternDestroy" );
		ObjectSetDestroy =
			(decltype( ObjectSetDestroy ))Sys::loadFunction( handle, "FcObjectSetDestroy" );
		FontSetDestroy =
			(decltype( FontSetDestroy ))Sys::loadFunction( handle, "FcFontSetDestroy" );
		PatternGetString =
			(decltype( PatternGetString ))Sys::loadFunction( handle, "FcPatternGetString" );
		PatternGetInteger =
			(decltype( PatternGetInteger ))Sys::loadFunction( handle, "FcPatternGetInteger" );

		return InitLoadConfigAndFonts && ConfigDestroy && Fini && PatternCreate &&
			   ObjectSetCreate && ObjectSetAdd && FontList && PatternDestroy && ObjectSetDestroy &&
			   FontSetDestroy && PatternGetString && PatternGetInteger;
	}

	void finish( FcConfig* config ) {
		if ( config )
			ConfigDestroy( config );
		Fini();
	}

	void unload() {
		if ( handle ) {
			Sys::unloadObject( handle );
			handle = nullptr;
		}
	}
};

#define FC_W( v ) FcLib::FC_WEIGHT_##v
#define FC_S( v ) FcLib::FC_SLANT_##v
#define FC_WI( v ) FcLib::FC_WIDTH_##v

static FontWeight fcWeightToFontWeight( int fcWeight ) {
	if ( fcWeight <= FC_W( THIN ) )
		return FontWeight::Thin;
	if ( fcWeight <= FC_W( EXTRALIGHT ) )
		return FontWeight::ExtraLight;
	if ( fcWeight <= FC_W( LIGHT ) )
		return FontWeight::Light;
	if ( fcWeight <= FC_W( REGULAR ) )
		return FontWeight::Normal;
	if ( fcWeight <= FC_W( MEDIUM ) )
		return FontWeight::Medium;
	if ( fcWeight <= FC_W( SEMIBOLD ) )
		return FontWeight::SemiBold;
	if ( fcWeight <= FC_W( BOLD ) )
		return FontWeight::Bold;
	if ( fcWeight <= FC_W( EXTRABOLD ) )
		return FontWeight::ExtraBold;
	return FontWeight::Black;
}

static FontStretch fcWidthToFontStretch( int fcWidth ) {
	if ( fcWidth <= FC_WI( ULTRACONDENSED ) )
		return FontStretch::UltraCondensed;
	if ( fcWidth <= FC_WI( EXTRACONDENSED ) )
		return FontStretch::ExtraCondensed;
	if ( fcWidth <= FC_WI( CONDENSED ) )
		return FontStretch::Condensed;
	if ( fcWidth <= FC_WI( SEMICONDENSED ) )
		return FontStretch::SemiCondensed;
	if ( fcWidth <= FC_WI( NORMAL ) )
		return FontStretch::Normal;
	if ( fcWidth <= FC_WI( SEMIEXPANDED ) )
		return FontStretch::SemiExpanded;
	if ( fcWidth <= FC_WI( EXPANDED ) )
		return FontStretch::Expanded;
	if ( fcWidth <= FC_WI( EXTRAEXPANDED ) )
		return FontStretch::ExtraExpanded;
	return FontStretch::UltraExpanded;
}

void SystemFontResolver::populateFontList() const {
	FcLib fc;
	if ( !fc.load() ) {
		populateFontListFallback();
		return;
	}
	FcLib::FcConfig* config = fc.InitLoadConfigAndFonts();
	if ( !config ) {
		fc.finish( nullptr );
		fc.unload();
		populateFontListFallback();
		return;
	}

	FcLib::FcPattern* pattern = fc.PatternCreate();
	FcLib::FcObjectSet* os = fc.ObjectSetCreate();
	if ( !pattern || !os ) {
		if ( pattern )
			fc.PatternDestroy( pattern );
		if ( os )
			fc.ObjectSetDestroy( os );
		fc.finish( config );
		fc.unload();
		populateFontListFallback();
		return;
	}

	bool objectSetOk = fc.ObjectSetAdd( os, "family" ) && fc.ObjectSetAdd( os, "file" ) &&
					   fc.ObjectSetAdd( os, "index" ) && fc.ObjectSetAdd( os, "weight" ) &&
					   fc.ObjectSetAdd( os, "width" ) && fc.ObjectSetAdd( os, "slant" ) &&
					   fc.ObjectSetAdd( os, "spacing" );
	if ( !objectSetOk ) {
		fc.PatternDestroy( pattern );
		fc.ObjectSetDestroy( os );
		fc.finish( config );
		fc.unload();
		populateFontListFallback();
		return;
	}

	FcLib::FcFontSet* fontSet = fc.FontList( config, pattern, os );

	fc.PatternDestroy( pattern );
	fc.ObjectSetDestroy( os );

	if ( !fontSet ) {
		fc.finish( config );
		fc.unload();
		populateFontListFallback();
		return;
	}

	for ( int i = 0; i < fontSet->nfont; ++i ) {
		FcLib::FcPattern* font = fontSet->fonts[i];

		FcLib::FcChar8* family = nullptr;
		if ( fc.PatternGetString( font, "family", 0, &family ) != FcLib::FcResultMatch || !family )
			continue;

		FcLib::FcChar8* file = nullptr;
		if ( fc.PatternGetString( font, "file", 0, &file ) != FcLib::FcResultMatch || !file )
			continue;

		int fcIndex = 0;
		fc.PatternGetInteger( font, "index", 0, &fcIndex );

		int fcWeight = FC_W( REGULAR );
		fc.PatternGetInteger( font, "weight", 0, &fcWeight );

		int fcWidth = FC_WI( NORMAL );
		fc.PatternGetInteger( font, "width", 0, &fcWidth );

		int fcSlant = FC_S( ROMAN );
		fc.PatternGetInteger( font, "slant", 0, &fcSlant );

		int fcSpacing = FcLib::FC_PROPORTIONAL;
		fc.PatternGetInteger( font, "spacing", 0, &fcSpacing );

		FontDesc desc;
		desc.family = reinterpret_cast<const char*>( family );
		desc.path = reinterpret_cast<const char*>( file );
		desc.faceIndex = fcIndex >= 0 ? static_cast<Uint32>( fcIndex & 0xFFFF ) : 0;
		desc.weight = fcWeightToFontWeight( fcWeight );
		desc.stretch = fcWidthToFontStretch( fcWidth );
		desc.italic = ( fcSlant == FC_S( ITALIC ) || fcSlant == FC_S( OBLIQUE ) );
		desc.monospace = ( fcSpacing == FcLib::FC_MONO );

		mFontList.push_back( desc );
	}

	fc.FontSetDestroy( fontSet );
	fc.finish( config );
	fc.unload();
}

// =====================================================================
// Platform: Android (XML fallback)
// =====================================================================
#elif EE_PLATFORM == EE_PLATFORM_ANDROID

void SystemFontResolver::populateFontList() const {
	// Android's NDK ASystemFontIterator lacks family name exposure,
	// and AFontMatcher only matches text to a single font.
	// Parsing fonts.xml is the standard way to get logical font families.
	static const char* fontPaths[] = { "/system/etc/fonts.xml", "/system/fonts/fonts.xml",
									   "/vendor/etc/fonts.xml", nullptr };

	const char* xmlPath = nullptr;
	for ( int i = 0; fontPaths[i]; ++i ) {
		if ( FileSystem::fileExists( fontPaths[i] ) ) {
			xmlPath = fontPaths[i];
			break;
		}
	}

	if ( !xmlPath )
		return;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file( xmlPath );
	if ( !result )
		return;

	pugi::xml_node families = doc.child( "familyset" );
	if ( !families )
		return;

	for ( pugi::xml_node familyNode : families.children( "family" ) ) {
		std::string familyName = familyNode.attribute( "name" ).as_string();
		if ( familyName.empty() )
			continue;

		for ( pugi::xml_node fontNode : familyNode.children( "font" ) ) {
			std::string fontFile = fontNode.text().as_string();
			if ( fontFile.empty() )
				continue;

			std::string fontPath = "/system/fonts/" + fontFile;
			if ( !FileSystem::fileExists( fontPath ) )
				fontPath = fontFile;

			std::string weightStr = fontNode.attribute( "weight" ).as_string();
			FontWeight weight = FontWeight::Normal;
			if ( !weightStr.empty() )
				weight = static_cast<FontWeight>( std::atoi( weightStr.c_str() ) );

			std::string styleStr = fontNode.attribute( "style" ).as_string();
			bool italic = ( styleStr == "italic" );

			FontDesc desc;
			desc.family = familyName;
			desc.path = fontPath;
			desc.faceIndex = 0;
			desc.weight = weight;
			desc.italic = italic;
			// Note: Android XML doesn't strictly provide a generic monospace flag in this node,
			// so you may need to default to false or infer from the family name.
			desc.monospace = ( familyName.find( "monospace" ) != std::string::npos );

			mFontList.push_back( desc );
		}
	}
}

// =====================================================================
// Platform: Haiku (Fallback)
// =====================================================================
#elif EE_PLATFORM == EE_PLATFORM_HAIKU

void SystemFontResolver::populateFontList() const {
	return populateFontListFallback();
}

// =====================================================================
// Platform: Emscripten / Unknown (no system font access)
// =====================================================================
#else

void SystemFontResolver::populateFontList() const {}

#endif

void SystemFontResolver::populateFontListFallback() const {
	// Added Haiku font paths so testing this fallback on Haiku actually finds files
	static const char* fontDirs[] = { "/usr/share/fonts",
									  "/usr/share/fonts/truetype",
									  "/usr/local/share/fonts",
#if EE_PLATFORM == EE_PLATFORM_HAIKU
									  "/system/data/fonts/ttfonts",
									  "/system/data/fonts/otfonts",
									  "/system/non-packaged/data/fonts",
#endif
									  nullptr };

	FT_Library ftLibrary;
	if ( FT_Init_FreeType( &ftLibrary ) != 0 )
		return;

	for ( int d = 0; fontDirs[d]; ++d ) {
		std::string dir( fontDirs[d] );

		if ( !FileSystem::isDirectory( dir ) )
			continue;

		auto files = FileSystem::filesGetInPath( dir, false, true );
		for ( const auto& file : files ) {
			std::string ext = FileSystem::fileExtension( file );
			if ( ext != "ttf" && ext != "otf" && ext != "ttc" && ext != "otc" && ext != "woff" &&
				 ext != "woff2" && ext != "bdf" && ext != "otb" )
				continue;

			FileSystem::dirAddSlashAtEnd( dir );
			std::string path = dir + file;

			// Temporarily load the face to get the total number of sub-faces
			FT_Face tempFace;
			if ( FT_New_Face( ftLibrary, path.c_str(), 0, &tempFace ) == 0 ) {
				long numFaces = tempFace->num_faces;
				FT_Done_Face( tempFace );

				// Loop through all faces in the file (Crucial for .ttc/.otc files)
				for ( long i = 0; i < numFaces; ++i ) {
					FT_Face face;
					if ( FT_New_Face( ftLibrary, path.c_str(), i, &face ) == 0 ) {
						FontDesc desc;
						desc.family = face->family_name ? face->family_name : "Unknown";
						desc.path = path;
						desc.faceIndex = static_cast<Uint32>( i );

						desc.weight = ( face->style_flags & FT_STYLE_FLAG_BOLD )
										  ? FontWeight::Bold
										  : FontWeight::Normal;
						desc.italic = ( face->style_flags & FT_STYLE_FLAG_ITALIC ) != 0;
						desc.monospace = FT_IS_FIXED_WIDTH( face ) != 0;

						mFontList.push_back( desc );
						FT_Done_Face( face );
					}
				}
			}
		}
	}

	FT_Done_FreeType( ftLibrary );
}

}} // namespace EE::Graphics
