#ifndef EE_UICUISKINSTATE_HPP
#define EE_UICUISKINSTATE_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class UISkin;

class EE_API UISkinState {
	public:
		enum UISkinStates {
			StateNormal = 0,
			StateFocus,
			StateSelected,
			StateMouseEnter,
			StateMouseExit,
			StateMouseDown,
			StateCount
		};

		static UISkinState * New( UISkin * skin );

		UISkinState( UISkin * Skin );

		~UISkinState();

		const Uint32& getState() const;

		void setState( const Uint32& State );

		UISkin * getSkin() const;

		void draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha );

		bool stateExists( const Uint32& State );

		const Uint32& getPrevState() const;
	protected:
		friend class UIControl;

		UISkin * 	mSkin;
		Uint32 		mCurState;
		Uint32		mLastState;

		void stateBack( const Uint32& State );

		void setPrevState();

		void setStateTypeSimple( const Uint32& State );

		void setStateTypeDefault( const Uint32& State );
};

}}

#endif

