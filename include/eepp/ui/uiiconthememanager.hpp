#ifndef EE_UI_UIICONTHEMEMANAGER_HPP
#define EE_UI_UIICONTHEMEMANAGER_HPP

#include <eepp/ui/uiicontheme.hpp>
#include <vector>

namespace EE { namespace UI {

class UIThemeManager;

class EE_API UIIconThemeManager {
  public:
	static std::string getIconNameFromFileName( const std::string& fileName,
												bool retOnlyWithExtension = false );

	static UIIconThemeManager* New();

	~UIIconThemeManager();

	UIIconThemeManager* add( UIIconThemePtr iconTheme );

	UIIconTheme* getCurrentTheme() const;

	UIIconThemeManager* setCurrentTheme( UIIconThemePtr currentTheme );

	UIIconTheme* getFallbackTheme() const;

	UIIconThemeManager* setFallbackTheme( UIIconThemePtr fallbackTheme );

	UIIcon* findIcon( const std::string& name );

	UIThemeManager* getFallbackThemeManager() const;

	UIIconThemeManager* setFallbackThemeManager( UIThemeManager* fallbackThemeManager );

	void remove( UIIconTheme* iconTheme );

  protected:
	std::vector<UIIconThemePtr> mIconThemes;
	UIIconTheme* mCurrentTheme{ nullptr };
	UIIconTheme* mFallbackTheme{ nullptr };
	UIThemeManager* mFallbackThemeManager{ nullptr };

	UIIconThemeManager();

	bool isPresent( UIIconTheme* iconTheme );
};

}} // namespace EE::UI

#endif // EE_UI_UIICONTHEMEMANAGER_HPP
