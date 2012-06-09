#ifndef EE_UICUISKINSIMPLE_HPP
#define EE_UICUISKINSIMPLE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuiskin.hpp>

namespace EE { namespace UI {

class EE_API cUISkinSimple : public cUISkin {
	public:
		cUISkinSimple( const std::string& Name );

		virtual ~cUISkinSimple();

		virtual void Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height, const Uint32& Alpha, const Uint32& State );

		void SetSkin( const Uint32& State );

		cShape * GetShape( const Uint32& State ) const;

		cUISkinSimple * Copy( const std::string& NewName, const bool& CopyColorsState = true );

		virtual cUISkin * Copy();
	protected:
		cShape * 	mShape[ cUISkinState::StateCount ];
		eeColorA	mTempColor;

		void StateNormalToState( const Uint32& State );
};

}}

#endif

