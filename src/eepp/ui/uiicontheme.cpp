#include <eepp/core/core.hpp>
#include <eepp/ui/uiicontheme.hpp>

namespace EE { namespace UI {

UIIconTheme* UIIconTheme::New( const std::string& name ) {
	return eeNew( UIIconTheme, ( name ) );
}

UIIconTheme::~UIIconTheme() {
	for ( auto icon : mIcons )
		eeDelete( icon.second );
}

UIIconTheme::UIIconTheme( const std::string& name ) : mName( name ) {}

UIIconTheme* UIIconTheme::add( UIIcon* icon ) {
	auto iconExists = mIcons.find( icon->getName() );
	if ( iconExists != mIcons.end() )
		eeDelete( iconExists->second );
	mIcons[icon->getName()] = icon;
	return this;
}

UIIconTheme* UIIconTheme::add( const std::unordered_map<std::string, UIIcon*>& icons ) {
	mIcons.insert( icons.begin(), icons.end() );
	return this;
}

const std::string& UIIconTheme::getName() const {
	return mName;
}

UIIcon* UIIconTheme::getIcon( const std::string& name ) const {
	auto it = mIcons.find( name );
	return it != mIcons.end() ? it->second : nullptr;
}

}} // namespace EE::UI
