#ifndef EE_UICUISKINSIMPLE_HPP
#define EE_UICUISKINSIMPLE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

class EE_API UISkinSimple : public UISkin {
	public:
		UISkinSimple( const std::string& getName );

		virtual ~UISkinSimple();

		virtual void Draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State );

		void SetSkin( const Uint32& State );

		SubTexture * GetSubTexture( const Uint32& State ) const;

		UISkinSimple * Copy( const std::string& NewName, const bool& CopyColorsState = true );

		virtual UISkin * Copy();
	protected:
		SubTexture * 	mSubTexture[ UISkinState::StateCount ];
		ColorA		mTempColor;

		void StateNormalToState( const Uint32& State );
};

}}

#endif

