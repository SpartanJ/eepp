#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include "base.hpp"
#include "../graphics/cshapegroup.hpp"
#include "../graphics/cfont.hpp"
#include "cuiskin.hpp"

namespace EE { namespace UI {

class EE_API cUITheme : public tResourceManager<cUISkin> {
	public:
		static cUITheme * LoadFromPath( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt = "png" );

		static cUITheme * LoadFromShapeGroup( cShapeGroup * ShapeGroup, const std::string& Name, const std::string NameAbbr );

		static void AddThemeElement( const std::string& Element );

		cUITheme( const std::string& Name, const std::string& Abbr, cFont * DefaultFont = NULL );

		virtual ~cUITheme();

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;

		const std::string& Abbr() const;

		virtual cUISkin * Add( cUISkin * Resource );

		void Font( cFont * Font );

		cFont * Font() const;

		const eeColorA& FontColor() const;

		const eeColorA& FontShadowColor() const;

		const eeColorA& FontOverColor() const;

		const eeColorA& FontSelectedColor() const;

		void FontColor( const eeColorA& Color );

		void FontShadowColor( const eeColorA& Color );

		void FontOverColor( const eeColorA& Color );

		void FontSelectedColor( const eeColorA& Color );
	protected:
		std::string 		mName;
		Uint32				mNameHash;
		std::string			mAbbr;
		cFont * 			mFont;
		eeColorA			mFontColor;
		eeColorA			mFontShadowColor;
		eeColorA			mFontOverColor;
		eeColorA			mFontSelectedColor;
	private:
		static bool SearchFilesOfElement( cShapeGroup * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt );

		static bool SearchFilesInGroup( cShapeGroup * SG, std::string Element, Uint32& IsComplex );
};

}}

#endif
