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

		void setDefaultFont( Font * Font );

		Font * getDefaultFont() const;

		void setTheme( const std::string& Theme );

		void setTheme( UITheme * Theme );

		void setDefaultTheme( UITheme * Theme );

		void setDefaultTheme( const std::string& Theme );

		UITheme * getDefaultTheme() const;

		void applyDefaultTheme( UIControl * Control );

		void setAutoApplyDefaultTheme( const bool& apply );

		const bool& getAutoApplyDefaultTheme() const;

		void setDefaultEffectsEnabled( const bool& Enabled );

		const bool& getDefaultEffectsEnabled() const;

		const Time& getControlsFadeInTime() const;

		void setControlsFadeInTime( const Time & Time );

		const Time& getControlsFadeOutTime() const;

		void setControlsFadeOutTime( const Time& Time );

		void setTooltipTimeToShow( const Time & Time );

		const Time& getTooltipTimeToShow() const;

		void setTooltipFollowMouse( const bool& Follow );

		const bool& getTooltipFollowMouse() const;

		void setCursorSize( const Sizei& Size );

		const Sizei& getCursorSize() const;

		TooltipStyleConfig getDefaultFontStyleConfig();
	protected:
		Font *				mFont;
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

