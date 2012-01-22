#ifndef EE_UICTHEMEMANAGER
#define EE_UICTHEMEMANAGER

#include "base.hpp"
#include "cuitheme.hpp"

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIThemeManager : public tResourceManager<cUITheme> {
	SINGLETON_DECLARE_HEADERS(cUIThemeManager)

	public:
		cUIThemeManager();

		virtual ~cUIThemeManager();

		void DefaultFont( cFont * Font );

		cFont * DefaultFont() const;

		void SetTheme( const std::string& Theme );

		void SetTheme( cUITheme * Theme );

		void DefaultTheme( cUITheme * Theme );

		void DefaultTheme( const std::string& Theme );

		cUITheme * DefaultTheme() const;

		void ApplyDefaultTheme( cUIControl * Control );

		void AutoApplyDefaultTheme( const bool& apply );

		const bool& AutoApplyDefaultTheme() const;

		void DefaultEffectsEnabled( const bool& Enabled );

		const bool& DefaultEffectsEnabled() const;

		const eeFloat& ControlsFadeInTime() const;

		void ControlsFadeInTime( const eeFloat& Time );

		const eeFloat& ControlsFadeOutTime() const;

		void ControlsFadeOutTime( const eeFloat& Time );

		void TooltipTimeToShow( const Uint32& Time );

		const Uint32& TooltipTimeToShow() const;

		void TooltipFollowMouse( const bool& Follow );

		const bool& TooltipFollowMouse() const;

		void CursorSize( const eeSize& Size );

		const eeSize& CursorSize() const;
	protected:
		cFont * 			mFont;
		cUITheme * 			mThemeDefault;
		bool				mAutoApplyDefaultTheme;

		bool				mEnableDefaultEffects;
		eeFloat				mFadeInTime;
		eeFloat				mFadeOutTime;

		Uint32				mTooltipTimeToShow;
		bool				mTooltipFollowMouse;

		eeSize				mCursorSize;
};

}}

#endif

