#ifndef EE_UICUISKINCOMPLEX_HPP
#define EE_UICUISKINCOMPLEX_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

class EE_API UISkinComplex : public UISkin {
	public:
		static std::string GetSideSuffix( const Uint32& Side );

		enum UISkinComplexSides {
			Left = 0,
			Right,
			Down,
			Up,
			UpLeft,
			UpRight,
			DownLeft,
			DownRight,
			Center,
			SideCount
		};

		UISkinComplex( const std::string& getName );

		virtual ~UISkinComplex();

		virtual void Draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State );

		void SetSkin( const Uint32& State );

		SubTexture * GetSubTexture( const Uint32& State ) const;

		SubTexture * GetSubTextureSide( const Uint32& State, const Uint32& Side );

		UISkinComplex * Copy( const std::string& NewName, const bool& CopyColorsState = true );

		virtual UISkin * Copy();
	protected:
		SubTexture * 	mSubTexture[ UISkinState::StateCount ][ SideCount ];
		ColorA		mTempColor;

		void StateNormalToState( const Uint32& State );
};

}}

#endif
