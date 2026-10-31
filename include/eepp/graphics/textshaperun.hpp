#pragma once

#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>

namespace EE::Graphics {

class Font;
class FontTrueType;

// helper class that divides the string into lines and font runs.
class EE_API TextShapeRun {
  public:
	TextShapeRun( String::View str, FontTrueType* font, Uint32 characterSize, Uint32 style,
				  Float outlineThickness, bool isRTL );

	String::View curRun() const;

	bool hasNext() const;

	std::size_t pos() const;

	std::size_t length() const;

	void next();

	bool runIsNewLine() const;

	FontTrueType* font();

  protected:
	void findNextEnd();

	std::size_t clusterEnd( std::size_t start ) const;

	std::size_t clusterStart( std::size_t last ) const;

	Font* clusterFont( std::size_t start, std::size_t end ) const;

	String::View mString;
	std::size_t mIndex{ 0 };
	std::size_t mLen{ 0 };
	Font* mFont{ nullptr };
	Font* mLFont{ nullptr };
	Uint32 mCharacterSize;
	Uint32 mStyle;
	Float mOutlineThickness;
	Font* mCurFont{ nullptr };
	Font* mStartFont{ nullptr };
	/* Right-to-left only: short segments stay inline, longer ones spill to the heap. */
	SmallVector<std::size_t, 8> mClusterStarts;
	bool mIsNewLine : 1 { false };
	bool mIsRTL : 1 { false };
};

} // namespace EE::Graphics
