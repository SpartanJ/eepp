#ifndef EE_UICTHEMEMANAGER
#define EE_UICTHEMEMANAGER

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuitheme.hpp>

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIThemeManager : public ResourceManager<cUITheme> {
	SINGLETON_DECLARE_HEADERS(cUIThemeManager)

	public:
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

		const Time& ControlsFadeInTime() const;

		void ControlsFadeInTime( const Time & Time );

		const Time& ControlsFadeOutTime() const;

		void ControlsFadeOutTime( const Time& Time );

		void TooltipTimeToShow( const Time & Time );

		const Time& TooltipTimeToShow() const;

		void TooltipFollowMouse( const bool& Follow );

		const bool& TooltipFollowMouse() const;

		void CursorSize( const eeSize& Size );

		const eeSize& CursorSize() const;
	protected:
		cFont * 			mFont;
		cUITheme * 			mThemeDefault;
		bool				mAutoApplyDefaultTheme;

		bool				mEnableDefaultEffects;
		Time				mFadeInTime;
		Time				mFadeOutTime;

		Time				mTooltipTimeToShow;
		bool				mTooltipFollowMouse;

		eeSize				mCursorSize;

		cUIThemeManager();
};

}}

#endif

