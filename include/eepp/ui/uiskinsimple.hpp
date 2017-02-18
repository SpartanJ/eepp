#ifndef EE_UICUISKINSIMPLE_HPP
#define EE_UICUISKINSIMPLE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

class EE_API UISkinSimple : public UISkin {
	public:
		UISkinSimple( const std::string& getName );

		virtual ~UISkinSimple();

		virtual void draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State );

		void setSkin( const Uint32& State );

		SubTexture * getSubTexture( const Uint32& State ) const;

		UISkinSimple * clone( const std::string& NewName, const bool& CopyColorsState = true );

		virtual UISkin * clone();
	protected:
		SubTexture * 	mSubTexture[ UISkinState::StateCount ];
		ColorA		mTempColor;

		void stateNormalToState( const Uint32& State );
};

}}

#endif

