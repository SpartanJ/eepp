#ifndef EE_UI_UIICONTHEME_HPP
#define EE_UI_UIICONTHEME_HPP

#include <eepp/graphics/drawable.hpp>
#include <unordered_map>

using namespace EE::Graphics;

namespace EE { namespace UI {

class EE_API UIIconTheme {
  public:
	static UIIconTheme* New( const std::string& name );

	UIIconTheme* add( const std::string& name, Drawable* drawable );

	UIIconTheme* add( const std::unordered_map<std::string, Drawable*>& icons );

	const std::string& getName() const;

	Drawable* getIcon( const std::string& name ) const;

  protected:
	std::string mName;
	std::unordered_map<std::string, Drawable*> mIcons;

	UIIconTheme( const std::string& name );
};

}} // namespace EE::UI

#endif // EE_UI_UIICONTHEME_HPP
