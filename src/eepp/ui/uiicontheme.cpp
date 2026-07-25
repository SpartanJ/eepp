#include <eepp/core/core.hpp>
#include <eepp/ui/uiicontheme.hpp>

namespace EE { namespace UI {

UIIconThemePtr UIIconTheme::New( const std::string& name ) {
	return UIIconThemePtr( eeNew( UIIconTheme, ( name ) ), ResourceDeleter<UIIconTheme>() );
}

UIIconTheme::~UIIconTheme() = default;

UIIconTheme::UIIconTheme( const std::string& name ) : mName( name ) {}

UIIconTheme* UIIconTheme::add( UIIconPtr icon ) {
	if ( icon )
		mIcons[icon->getName()] = std::move( icon );
	return this;
}

UIIconTheme* UIIconTheme::add( const std::unordered_map<std::string, UIIconPtr>& icons ) {
	mIcons.insert( icons.begin(), icons.end() );
	return this;
}

const std::string& UIIconTheme::getName() const {
	return mName;
}

UIIcon* UIIconTheme::getIcon( const std::string& name ) const {
	auto it = mIcons.find( name );
	return it != mIcons.end() ? it->second.get() : nullptr;
}

}} // namespace EE::UI
