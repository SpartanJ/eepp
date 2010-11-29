#ifndef EE_UICUISKINCOMPLEX_HPP
#define EE_UICUISKINCOMPLEX_HPP

#include "base.hpp"
#include "cuiskin.hpp"

namespace EE { namespace UI {

class EE_API cUISkinComplex : public cUISkin {
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

		cUISkinComplex( const std::string& Name );

		virtual ~cUISkinComplex();

		virtual void Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height, const Uint32& Alpha, const Uint32& State );

		void SetSkin( const Uint32& State );

		cShape * GetShape( const Uint32& State ) const;

		cShape * GetShapeSide( const Uint32& State, const Uint32& Side );

		cUISkinComplex * Copy( const std::string& NewName, const bool& CopyColorsState = true );

		virtual cUISkin * Copy();
	protected:
		cShape * 	mShape[ cUISkinState::StateCount ][ SideCount ];
		eeColorA	mTempColor;

		void StateNormalToState( const Uint32& State );
};

}}

#endif
