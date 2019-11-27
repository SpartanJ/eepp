#ifndef EE_UI_CSS_STYLESHEETLENGTH_HPP
#define EE_UI_CSS_STYLESHEETLENGTH_HPP

#include <string>
#include <eepp/config.hpp>
#include <eepp/math/size.hpp>
#include <eepp/scene/scenenode.hpp>

using namespace EE::Math;

namespace EE { namespace UI { namespace CSS {

class StyleSheetLength {
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
		};

		static Unit unitFromString( std::string unitStr );

		StyleSheetLength();

		StyleSheetLength( std::string val, const Float& defaultValue = 0 );

		StyleSheetLength( const StyleSheetLength& val );

		void setValue( const Float& val, const Unit& units );

		const Float& getValue() const;

		const Unit& getUnit() const;

		Float asPixels( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi, const Float& elFontSize = 12, const Float& globalFontSize = 12 ) const;

		Float asDp( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi, const Float& elFontSize = 12, const Float& globalFontSize = 12 ) const;

		bool operator==( const StyleSheetLength& val );

		StyleSheetLength& operator=( const StyleSheetLength& val );

		StyleSheetLength& operator=( const Float& val );

		static StyleSheetLength fromString( std::string str, const Float& defaultValue = 0 );
	protected:
		Unit mUnit;
		Float mValue;
};

typedef StyleSheetLength CSSLength;

}}}

#endif
