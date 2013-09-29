#ifndef EE_UICTHEMEMANAGER
#define EE_UICTHEMEMANAGER

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuitheme.hpp>

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIThemeManager : public tResourceManager<cUITheme> {
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

		const cTime& ControlsFadeInTime() const;

		void ControlsFadeInTime( const cTime & Time );

		const cTime& ControlsFadeOutTime() const;

		void ControlsFadeOutTime( const cTime& Time );

		void TooltipTimeToShow( const cTime & Time );

		const cTime& TooltipTimeToShow() const;

		void TooltipFollowMouse( const bool& Follow );

		const bool& TooltipFollowMouse() const;

		void CursorSize( const eeSize& Size );

		const eeSize& CursorSize() const;
	protected:
		cFont * 			mFont;
		cUITheme * 			mThemeDefault;
		bool				mAutoApplyDefaultTheme;

		bool				mEnableDefaultEffects;
		cTime				mFadeInTime;
		cTime				mFadeOutTime;

		cTime				mTooltipTimeToShow;
		bool				mTooltipFollowMouse;

		eeSize				mCursorSize;

		cUIThemeManager();
};

}}

#endif

