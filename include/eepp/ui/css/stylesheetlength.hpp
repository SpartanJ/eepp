#ifndef EE_UI_CSS_STYLESHEETLENGTH_HPP
#define EE_UI_CSS_STYLESHEETLENGTH_HPP

#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/math/size.hpp>
#include <eepp/scene/scenenode.hpp>
#include <string>
#include <string_view>

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

using namespace EE::Math;

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetLength {
  public:
	enum Unit {
		Percentage,
		In,
		Cm,
		Mm,
		Em,
		Ex,
		Pt,
		Pc,
		Px,
		Dpi,
		Dp,
		Dpcm,
		Vw,
		Vh,
		Vmin,
		Vmax,
		Rem,
		Dprd,
		Dpru,
		Dpr,
		Ch,
		Clamp,
		Min,
		Max,
		Calc,
	};

	using Arguments = SmallVector<std::string, 4>;

	static Unit unitFromString( std::string_view unitStr );

	static std::string unitToString( const Unit& unit );

	static bool isLength( std::string_view unitStr );

	static bool isPercentage( std::string_view val );

	static StyleSheetLength fromString( const std::string& str, const Float& defaultValue = 0,
										bool pxAsDp = false );

	StyleSheetLength();

	StyleSheetLength( const Float& val, const Unit& unit );

	StyleSheetLength( const std::string& val, const Float& defaultValue = 0 );

	StyleSheetLength( const StyleSheetLength& val );

	void setValue( const Float& val, const Unit& units );

	const Float& getValue() const;

	const Unit& getUnit() const;

	Float asPixels( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi,
					const Float& elFontSize = 12, const Float& globalFontSize = 12,
					Graphics::Font* font = nullptr ) const;

	Float asDp( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi,
				const Float& elFontSize = 12, const Float& globalFontSize = 12,
				Graphics::Font* font = nullptr ) const;

	bool operator==( const StyleSheetLength& val ) const;

	bool operator!=( const StyleSheetLength& val ) const;

	StyleSheetLength& operator=( const StyleSheetLength& val );

	StyleSheetLength& operator=( const Float& val );

	std::string toString() const;

	const Arguments& getArgs() const;

	void setArgs( const Arguments& args );

  protected:
	static bool isFunctionString( std::string_view str );

	static bool parseFunction( const std::string& str, Unit& outUnit, Arguments& outArgs );

	Float resolveFunction( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi,
						   const Float& elFontSize, const Float& globalFontSize,
						   Graphics::Font* font ) const;

	Float resolveCalc( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi,
					   const Float& elFontSize, const Float& globalFontSize,
					   Graphics::Font* font ) const;

	Unit mUnit;
	Float mValue;
	Arguments mArgs;
};

}}} // namespace EE::UI::CSS

#endif
