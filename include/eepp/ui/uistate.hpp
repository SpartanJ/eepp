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

		static const char * getStateName( const Uint32& State );

		static int getStateNumber(const std::string & State);

		static bool isStateName( const std::string& State );

		UIState();

		virtual ~UIState();

		const Uint32& getState() const;

		void setState( const Uint32& State );

		void pushState( const Uint32& State );

		void popState( const Uint32& State );

		virtual bool stateExists( const Uint32& State ) = 0;

		Uint32 getCurrentState() const;
	protected:
		Uint32 		mState;
		Uint32		mCurrentState;

		void updateState();

		virtual void onStateChange();
};

}}

#endif

