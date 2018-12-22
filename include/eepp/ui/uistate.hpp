#ifndef EE_UI_UISTATE_HPP
#define EE_UI_UISTATE_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class UISkin;

class EE_API UIState {
	public:
		enum UIStates {
			StateNormal = 0,
			StateFocus,
			StateSelected,
			StateHover,
			StatePressed,
			StateSelectedHover,
			StateSelectedPressed,
			StateDisabled,
			StateCount
		};

		enum UIStatesFlags {
			StateFlagNormal           = 1 << StateNormal,
			StateFlagFocus	          = 1 << StateFocus,
			StateFlagSelected         = 1 << StateSelected,
			StateFlagHover            = 1 << StateHover,
			StateFlagPressed          = 1 << StatePressed,
			StateFlagSelectedHover    = StateFlagSelected | StateFlagHover,
			StateFlagSelectedPressed  = StateFlagSelected | StateFlagPressed,
			StateFlagDisabled         = 1 << StateDisabled,
			StateFlagCount            = StateCount
		};

		static const char * getSkinStateName( const Uint32& State );

		static int getStateNumber(const std::string & State);

		static bool isStateName( const std::string& State );

		static UIState * New( UISkin * skin );

		explicit UIState( UISkin * Skin );

		~UIState();

		const Uint32& getState() const;

		void setState( const Uint32& State );

		void pushState( const Uint32& State );

		void popState( const Uint32& State );

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

