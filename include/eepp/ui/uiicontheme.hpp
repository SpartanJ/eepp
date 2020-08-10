#ifndef EE_UI_UIICONTHEME_HPP
#define EE_UI_UIICONTHEME_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/ui/uiicon.hpp>
#include <unordered_map>

using namespace EE::Graphics;

namespace EE { namespace UI {

class EE_API UIIconTheme {
  public:
	static UIIconTheme* New( const std::string& name );

	~UIIconTheme();

	UIIconTheme* add( UIIcon* icon );

	UIIconTheme* add( const std::unordered_map<std::string, UIIcon*>& icons );

	const std::string& getName() const;

	UIIcon* getIcon( const std::string& name ) const;

  protected:
	std::string mName;
	std::unordered_map<std::string, UIIcon*> mIcons;

	UIIconTheme( const std::string& name );
};

}} // namespace EE::UI

#endif // EE_UI_UIICONTHEME_HPP
