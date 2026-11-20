#ifndef EE_UICTHEMEMANAGER
#define EE_UICTHEMEMANAGER

#include <eepp/graphics/resourcescope.hpp>
#include <eepp/ui/base.hpp>
#include <eepp/ui/uitheme.hpp>
#include <optional>

namespace EE { namespace UI {

class UINode;
class UISceneNode;

class EE_API UIThemeManager {
  public:
	static UIThemeManager* New();

	virtual ~UIThemeManager();

	UITheme* add( UIThemePtr theme );

	bool remove( UITheme* theme );

	bool removeById( const String::HashType& id );

	bool removeByName( const std::string& name );

	UIThemeManager* setResourceScope( Graphics::ResourceScopePtr resourceScope );

	UIThemeManager* setDefaultFont( Font* Font );

	Font* getDefaultFont() const;

	/** Sets the manager's font size in pixels at its current density, overriding the theme default.
	 * The size scales with subsequent density changes. */
	UIThemeManager* setDefaultFontSize( const Float& fontSize );

	const Float& getDefaultFontSize() const;

	/** Refreshes this scene's resolved defaults without modifying its shared theme. */
	void onPixelDensityChange( Float density );

	UIThemeManager* setDefaultTheme( UITheme* Theme );

	UIThemeManager* setDefaultTheme( UIThemePtr theme );

	UIThemeManager* setDefaultTheme( const std::string& Theme );

	UITheme* getDefaultTheme() const;

	/** @return An owning handle to the default theme, or an empty handle when unset. */
	UIThemePtr getDefaultThemeHandle() const;

	UITheme* getById( const String::HashType& id ) const;

	UITheme* getByName( const std::string& name ) const;

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
	mutable Float mFontSize;
	Float mFallbackFontSizeDp{ 12.f };
	Float mPixelDensity;
	UIThemePtr mThemeDefault;
	UnorderedMap<String::HashType, UIThemePtr> mThemes;
	bool mAutoApplyDefaultTheme;

	bool mEnableDefaultEffects;
	Time mFadeInTime;
	Time mFadeOutTime;

	Time mTooltipTimeToShow;
	bool mTooltipFollowMouse;

	Sizei mCursorSize;
	Graphics::ResourceScopePtr mResourceScope;

	UIThemeManager();

  private:
	friend class UISceneNode;
	std::optional<Float> mDefaultFontSizeOverrideDp;
};

}} // namespace EE::UI

#endif
