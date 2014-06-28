#ifndef EE_UICTHEMEMANAGER
#define EE_UICTHEMEMANAGER

#include <eepp/ui/base.hpp>
#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

class UIControl;

class EE_API UIThemeManager : public ResourceManager<UITheme> {
	SINGLETON_DECLARE_HEADERS(UIThemeManager)

	public:
		virtual ~UIThemeManager();

		void DefaultFont( Font * Font );

		Font * DefaultFont() const;

		void SetTheme( const std::string& Theme );

		void SetTheme( UITheme * Theme );

		void DefaultTheme( UITheme * Theme );

		void DefaultTheme( const std::string& Theme );

		UITheme * DefaultTheme() const;

		void ApplyDefaultTheme( UIControl * Control );

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

		void CursorSize( const Sizei& Size );

		const Sizei& CursorSize() const;
	protected:
		Font * 			mFont;
		UITheme * 			mThemeDefault;
		bool				mAutoApplyDefaultTheme;

		bool				mEnableDefaultEffects;
		Time				mFadeInTime;
		Time				mFadeOutTime;

		Time				mTooltipTimeToShow;
		bool				mTooltipFollowMouse;

		Sizei				mCursorSize;

		UIThemeManager();
};

}}

#endif

