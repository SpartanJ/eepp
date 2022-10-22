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
		StateChecked,
		StateFocusWithin,
		StateCount
	};

	enum UIStatesFlags {
		StateFlagNormal = 1 << StateNormal,
		StateFlagFocus = 1 << StateFocus,
		StateFlagSelected = 1 << StateSelected,
		StateFlagHover = 1 << StateHover,
		StateFlagPressed = 1 << StatePressed,
		StateFlagSelectedHover = StateFlagSelected | StateFlagHover,
		StateFlagSelectedPressed = StateFlagSelected | StateFlagPressed,
		StateFlagDisabled = 1 << StateDisabled,
		StateFlagChecked = 1 << StateChecked,
		StateFlagFocusWithin = 1 << StateFocusWithin,
		StateFlagCount = StateCount
	};

	static const char* getStateName( const Uint32& State );

	static int getStateNumber( const std::string& State );

	static const char* getStateNameFromStateFlag( const Uint32& stateFlag );

	static const Uint32& getStateFlag( const Uint32& stateIndex );

	static Uint32 getStateFlagFromName( const std::string& name );

	static bool isStateName( const std::string& State );

	UIState();

	virtual ~UIState();

	const Uint32& getState() const;

	void setState( const Uint32& State );

	void pushState( const Uint32& State );

	void popState( const Uint32& State );

	virtual bool stateExists( const Uint32& State ) const = 0;

	const Uint32& getCurrentState() const;

	const Uint32& getPreviousState() const;

  protected:
	Uint32 mState;
	Uint32 mCurrentState;
	Uint32 mPreviousState;

	virtual void updateState() = 0;

	virtual void onStateChange();
};

}} // namespace EE::UI

#endif
