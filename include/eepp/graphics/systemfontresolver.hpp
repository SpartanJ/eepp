#ifndef EE_GRAPHICS_SYSTEMFONTRESOLVER_HPP
#define EE_GRAPHICS_SYSTEMFONTRESOLVER_HPP

#include <eepp/config.hpp>
#include <eepp/graphics/base.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/singleton.hpp>

#include <atomic>
#include <string>
#include <vector>

namespace EE { namespace Graphics {

enum class FontWeight : Uint16 {
	Thin = 100,
	ExtraLight = 200,
	Light = 300,
	Normal = 400,
	Medium = 500,
	SemiBold = 600,
	Bold = 700,
	ExtraBold = 800,
	Black = 900
};

enum class FontStretch : Uint8 {
	UltraCondensed = 1,
	ExtraCondensed = 2,
	Condensed = 3,
	SemiCondensed = 4,
	Normal = 5,
	SemiExpanded = 6,
	Expanded = 7,
	ExtraExpanded = 8,
	UltraExpanded = 9
};

enum class GenericFamily : Uint8 {
	None,
	Serif,
	SansSerif,
	Monospace,
	Cursive,
	Fantasy,
	SystemUi,
	Emoji
};

struct FontQuery {
	std::string family;
	FontWeight weight{ FontWeight::Normal };
	FontStretch stretch{ FontStretch::Normal };
	bool italic{ false };
};

struct FontDesc {
	std::string family;
	std::string path;
	Uint32 faceIndex{ 0 };
	FontWeight weight{ FontWeight::Normal };
	FontStretch stretch{ FontStretch::Normal };
	bool italic{ false };
	bool monospace{ false };

	std::string getFileKey() const { return path + "#" + String::toString( faceIndex ); }

	std::string getStyleKey() const {
		return String::toLower( family ) + "#" + String::toString( static_cast<Uint32>( weight ) ) +
			   "#" + String::toString( static_cast<Uint32>( stretch ) ) + "#" +
			   String::toString( italic ? 1 : 0 ) + "#" + String::toString( faceIndex );
	}

	bool sameFile( const FontDesc& other ) const {
		return path == other.path && faceIndex == other.faceIndex;
	}

	bool sameFile( const std::string& otherPath, Uint32 otherFaceIndex = 0 ) const {
		return path == otherPath && faceIndex == otherFaceIndex;
	}

	bool sameStyle( const FontDesc& other ) const { return getStyleKey() == other.getStyleKey(); }

	bool operator==( const FontDesc& other ) const {
		return family == other.family && path == other.path && faceIndex == other.faceIndex &&
			   weight == other.weight && stretch == other.stretch && italic == other.italic &&
			   monospace == other.monospace;
	}

	bool operator!=( const FontDesc& other ) const { return !( *this == other ); }
};

class EE_API SystemFontResolver {
	SINGLETON_DECLARE_HEADERS( SystemFontResolver )

  public:
	~SystemFontResolver();

	FontDesc resolve( const FontQuery& query );

	FontDesc resolveFromNamesList( const std::string& namesList, FontWeight weight, bool italic );

	FontDesc resolveGeneric( GenericFamily generic, FontWeight weight, bool italic );

	FontDesc getSystemFont() const;

	FontDesc getSystemMonospaceFont() const;

	FontDesc getFallbackForCodepoint( Uint32 codepoint, FontWeight weight, bool italic );

	bool fontContainsCodepoint( const std::string& path, Uint32 codepoint );

	void invalidateCache();

	void ensureFontListPopulated() const;

	bool isLoading() const { return mFontListLoading; }

	std::vector<FontDesc> enumerate();

	std::vector<FontDesc> enumerateFamily( const std::string& family );

	template <typename Fn> void forEach( Fn&& fn ) {
		System::Lock lock( mMutex );
		for ( const auto& desc : mFontList )
			fn( desc );
	}

	static void setEnabled( bool enabled );

	static bool isEnabled();

	static GenericFamily genericFamilyFromName( const std::string& name );

  protected:
	SystemFontResolver();

  private:
	void populateFontList() const;

	void populateFontListFallback() const;

	void populateGenericFallbacks() const;

	static int scoreMatch( const FontQuery& query, const FontDesc& candidate );

	mutable System::Mutex mMutex;
	mutable std::vector<FontDesc> mFontList;
	mutable bool mFontListPopulated{ false };
	mutable std::atomic<bool> mFontListLoading{ false };

	static Uint64 makeCacheKey( const std::string& normFamily, FontWeight weight,
								FontStretch stretch, bool italic );

	mutable UnorderedMap<Uint64, FontDesc> mResolveCache;

	mutable UnorderedMap<Uint32, FontDesc> mGenericCache;

	mutable UnorderedMap<Uint32, std::string> mCodepointFallbackCache;

	struct GenericEntry {
		GenericFamily generic;
		FontDesc desc;
	};
	mutable std::vector<GenericEntry> mGenericFallbacks;

	static bool sEnabled;
};

}} // namespace EE::Graphics

#endif
