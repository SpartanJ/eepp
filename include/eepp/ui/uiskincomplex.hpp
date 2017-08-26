#ifndef EE_UICUISKINCOMPLEX_HPP
#define EE_UICUISKINCOMPLEX_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace Graphics {
class Drawable;
}}

namespace EE { namespace UI {

class EE_API UISkinComplex : public UISkin {
	public:
		static UISkinComplex * New( const std::string& name );

		static std::string getSideSuffix( const Uint32& Side );

		static bool isSideSuffix( const std::string& suffix );

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

		UISkinComplex( const std::string& name );

		virtual ~UISkinComplex();

		virtual void draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State );

		void setSkin( const Uint32& State );

		bool stateExists( const Uint32& state );

		Sizei getSideSize( const Uint32& State, const Uint32& Side );

		UISkinComplex * clone( const std::string& NewName, const bool& CopyColorsState = true );

		virtual UISkin * clone();

		Sizei getSize( const Uint32& state );

		Rect getBorderSize( const Uint32 &state );
	protected:
		Drawable * 	mDrawable[ UISkinState::StateCount ][ SideCount ];
		Color		mTempColor;
		Sizei		mSize[ UISkinState::StateCount ];
		Rect		mBorderSize[ UISkinState::StateCount ];

		void cacheSize();

		void stateNormalToState( const Uint32& State );
};

}}

#endif
