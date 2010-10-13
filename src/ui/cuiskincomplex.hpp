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

		virtual void Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height );

		void SetSkin( const Uint32& State );

		cShape * GetShape( const Uint32& State ) const;

		virtual void SetState( const Uint32& State );

		cUISkinComplex * Copy( const std::string& NewName, const bool& CopyColorsState = true );
	protected:
		cShape * 	mShape[ StateCount ][ SideCount ];

		void StateNormalToState( const Uint32& State );
};

}}

#endif
