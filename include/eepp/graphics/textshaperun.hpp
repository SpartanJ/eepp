#pragma once

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>

namespace EE::Graphics {

class Font;
class FontTrueType;

// helper class that divides the string into lines and font runs.
class EE_API TextShapeRun {
  public:
	TextShapeRun( String::View str, FontTrueType* font, Uint32 characterSize, Uint32 style,
				  Float outlineThickness );

	String::View curRun() const;

	bool hasNext() const;

	std::size_t pos() const;

	void next();

	bool runIsNewLine() const;

	FontTrueType* font();

  protected:
	void findNextEnd();

	String::View mString;
	std::size_t mIndex{ 0 };
	std::size_t mLen{ 0 };
	Font* mFont{ nullptr };
	Uint32 mCharacterSize;
	Uint32 mStyle;
	Float mOutlineThickness;
	Font* mCurFont{ nullptr };
	Font* mStartFont{ nullptr };
	bool mIsNewLine{ false };
};

} // namespace EE::Graphics
