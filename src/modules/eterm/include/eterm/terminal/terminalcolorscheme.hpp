#ifndef ETERM_TERMINALCOLORSCHEME_HPP
#define ETERM_TERMINALCOLORSCHEME_HPP

#include <eepp/system/color.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>
#include <string>
#include <vector>

using namespace EE;
using namespace EE::System;

namespace eterm { namespace Terminal {

class TerminalColorScheme {
  public:
	static TerminalColorScheme getDefault();

	static std::vector<TerminalColorScheme> loadFromStream( IOStream& stream );

	static std::vector<TerminalColorScheme> loadFromFile( const std::string& path );

	static std::vector<TerminalColorScheme> loadFromMemory( const void* data,
															std::size_t sizeInBytes );

	static std::vector<TerminalColorScheme> loadFromPack( Pack* pack, std::string filePackPath );

	struct Style {
		Style(){};
		Style( const Color& color ) : color( color ) {}
		Style( const Color& color, const Color& background, const Uint32& style ) :
			color( color ), background( background ), style( style ) {}
		Color color{ Color::White };
		Color background{ Color::Transparent };
		Uint32 style{ 0 };
	};

	TerminalColorScheme( const std::string& name, const Color& foreground, const Color& background,
						 const Color& cursor, const std::vector<Color> palette );

	const std::string& getName() const;

	const Color& getForeground() const;

	const Color& getBackground() const;

	const Color& getCursor() const;

	const std::vector<Color>& getPalette() const;

	const Color& getPaletteIndex( const size_t& index ) const;

	size_t getPaletteSize() const;

  protected:
	std::string mName;
	Color mForeground;
	Color mBackground;
	Color mCursor;
	std::vector<Color> mPalette;

	TerminalColorScheme();

	void setName( const std::string& name );
};

}} // namespace eterm::Terminal

#endif // ETERM_TERMINALCOLORSCHEME_HPP
