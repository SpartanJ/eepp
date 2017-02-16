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

		void defaultFont( Font * Font );

		Font * defaultFont() const;

		void setTheme( const std::string& Theme );

		void setTheme( UITheme * Theme );

		void defaultTheme( UITheme * Theme );

		void defaultTheme( const std::string& Theme );

		UITheme * defaultTheme() const;

		void applyDefaultTheme( UIControl * Control );

		void autoApplyDefaultTheme( const bool& apply );

		const bool& autoApplyDefaultTheme() const;

		void defaultEffectsEnabled( const bool& Enabled );

		const bool& defaultEffectsEnabled() const;

		const Time& controlsFadeInTime() const;

		void controlsFadeInTime( const Time & Time );

		const Time& controlsFadeOutTime() const;

		void controlsFadeOutTime( const Time& Time );

		void tooltipTimeToShow( const Time & Time );

		const Time& tooltipTimeToShow() const;

		void tooltipFollowMouse( const bool& Follow );

		const bool& tooltipFollowMouse() const;

		void cursorSize( const Sizei& Size );

		const Sizei& cursorSize() const;
	protected:
		Font * 			mFont;
		UITheme * 			mThemeDefault;
		bool				mautoApplyDefaultTheme;

		bool				mEnableDefaultEffects;
		Time				mFadeInTime;
		Time				mFadeOutTime;

		Time				mtooltipTimeToShow;
		bool				mtooltipFollowMouse;

		Sizei				mcursorSize;

		UIThemeManager();
};

}}

#endif

