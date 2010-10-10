#ifndef EE_UICUISKIN_HPP
#define EE_UICUISKIN_HPP

#include "base.hpp"

namespace EE { namespace UI {

class cUITheme;

class EE_API cUISkin {
	public:
		enum UISkinStates {
			StateNormal = 0,
			StateFocus,
			StateLostFocus,
			StateMouseEnter,
			StateMouseExit,
			StateMouseDown,
			StateCount
		};

		static const char * GetSkinStateName( const Uint32& State );

		cUISkin( const std::string& Name );

		virtual ~cUISkin();

		virtual void Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height ) = 0;

		virtual void SetSkin( const Uint32& State ) = 0;

		virtual cShape * GetShape( const Uint32& State ) const = 0;

		virtual void SetColor( const Uint32& State, const eeColorA& Color );

		virtual const eeColorA& GetColor( const Uint32& State ) const;

		virtual void SetState( const Uint32& State );

		virtual void SetSkins();

		const Uint32& GetState() const;

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;

		cUITheme * Theme() const;

		void Theme( cUITheme * theme );
	protected:
		friend class cUIControl;

		std::string mName;
		Uint32		mNameHash;
		Uint32 		mCurState;
		Uint32		mLastState;
		Uint32		mColorDefault;
		eeColorA 	mColor[ StateCount ];
		cUITheme * 	mTheme;

		void StateBack( const Uint32& State );

		void SetPrevState();
};

}}

#endif
