#ifndef EE_UICTHEMEMANAGER
#define EE_UICTHEMEMANAGER

#include <eepp/ui/base.hpp>
#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

class UINode;

class EE_API UIThemeManager : public ResourceManager<UITheme> {
  public:
	static UIThemeManager* New();

	virtual ~UIThemeManager();

	UIThemeManager* setDefaultFont( Font* Font );

	Font* getDefaultFont() const;

	UIThemeManager* setDefaultFontSize( const Float& fontSize );

	const Float& getDefaultFontSize() const;

	UIThemeManager* setDefaultTheme( UITheme* Theme );

	UIThemeManager* setDefaultTheme( const std::string& Theme );

	UITheme* getDefaultTheme() const;

	UIThemeManager* applyDefaultTheme( UINode* node );

	UIThemeManager* setAutoApplyDefaultTheme( const bool& apply );

	const bool& getAutoApplyDefaultTheme() const;

	UIThemeManager* setDefaultEffectsEnabled( const bool& Enabled );

	const bool& getDefaultEffectsEnabled() const;

	const Time& getWidgetsFadeInTime() const;

	UIThemeManager* setWidgetsFadeInTime( const Time& Time );

	const Time& getWidgetsFadeOutTime() const;

	UIThemeManager* setWidgetsFadeOutTime( const Time& Time );

	UIThemeManager* setTooltipTimeToShow( const Time& Time );

	const Time& getTooltipTimeToShow() const;

	UIThemeManager* setTooltipFollowMouse( const bool& Follow );

	const bool& getTooltipFollowMouse() const;

	UIThemeManager* setCursorSize( const Sizei& Size );

	const Sizei& getCursorSize() const;

  protected:
	Font* mFont;
	Float mFontSize;
	UITheme* mThemeDefault;
	bool mAutoApplyDefaultTheme;

	bool mEnableDefaultEffects;
	Time mFadeInTime;
	Time mFadeOutTime;

	Time mTooltipTimeToShow;
	bool mTooltipFollowMouse;

	Sizei mCursorSize;

	UIThemeManager();
};

}} // namespace EE::UI

#endif
