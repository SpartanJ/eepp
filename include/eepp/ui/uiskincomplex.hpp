#ifndef EE_UICUISKINCOMPLEX_HPP
#define EE_UICUISKINCOMPLEX_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

class EE_API UISkinComplex : public UISkin {
	public:
		static std::string getSideSuffix( const Uint32& Side );

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

		virtual void draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State );

		void setSkin( const Uint32& State );

		SubTexture * getSubTexture( const Uint32& State ) const;

		bool stateExists( const Uint32& state );

		SubTexture * getSubTextureSide( const Uint32& State, const Uint32& Side );

		UISkinComplex * clone( const std::string& NewName, const bool& CopyColorsState = true );

		virtual UISkin * clone();

		Sizei getSize( const Uint32& state );

		Sizei getBorderSize( const Uint32 &state );
	protected:
		SubTexture * 	mSubTexture[ UISkinState::StateCount ][ SideCount ];
		ColorA		mTempColor;
		Sizei		mSize[ UISkinState::StateCount ];
		Sizei		mBorderSize[ UISkinState::StateCount ];

		void cacheSize();

		void stateNormalToState( const Uint32& State );
};

}}

#endif
