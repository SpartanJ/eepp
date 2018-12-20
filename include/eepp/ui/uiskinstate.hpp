#ifndef EE_UICUISKINSTATE_HPP
#define EE_UICUISKINSTATE_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class UISkin;

class EE_API UISkinState {
	public:
		enum UISkinStates {
			StateNormal		= 0,
			StateFocus		= 1,
			StateSelected	= 2,
			StateHover		= 3,
			StatePressed	= 4,
			StateDisabled	= 5,
			StateCount		= 6
		};

		static UISkinState * New( UISkin * skin );

		UISkinState( UISkin * Skin );

		~UISkinState();

		const Uint32& getState() const;

		void setState( const Uint32& State );

		void unsetState( const Uint32& State );

		UISkin * getSkin() const;

		void draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha );

		bool stateExists( const Uint32& State );

		Uint32 getCurrentState() const;
	protected:
		friend class UINode;

		UISkin * 	mSkin;
		Uint32 		mState;
		Uint32		mCurrentState;

		void updateState();
};

}}

#endif

