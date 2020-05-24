#include <eepp/ui/doc/syntaxcolorscheme.hpp>

namespace EE { namespace UI { namespace Doc {

SyntaxColorScheme SyntaxColorScheme::getDefault() {
	return {"lite-theme",
			{
				{"normal", Color( "#e1e1e6" )},
				{"symbol", Color( "#e1e1e6" )},
				{"comment", Color( "#676b6f" )},
				{"keyword", Color( "#E58AC9" )},
				{"keyword2", Color( "#F77483" )},
				{"number", Color( "#FFA94D" )},
				{"literal", Color( "#FFA94D" )},
				{"string", Color( "#f7c95c" )},
				{"operator", Color( "#93DDFA" )},
				{"function", Color( "#93DDFA" )},
			}};
}

SyntaxColorScheme::SyntaxColorScheme() {}

SyntaxColorScheme::SyntaxColorScheme( const std::string& name,
									  const std::unordered_map<std::string, Color>& colors ) :
	mName( name ), mColors( colors ) {}

const Color& SyntaxColorScheme::getColor( const std::string& type ) const {
	auto it = mColors.find( type );
	if ( it != mColors.end() )
		return it->second;
	return Color::White;
}

void SyntaxColorScheme::setColors( const std::unordered_map<std::string, Color>& colors ) {
	mColors.insert( colors.begin(), colors.end() );
}

void SyntaxColorScheme::setColor( const std::string& type, const Color& color ) {
	mColors[type] = color;
}

const std::string& SyntaxColorScheme::getName() const {
	return mName;
}

void SyntaxColorScheme::setName( const std::string& name ) {
	mName = name;
}

}}} // namespace EE::UI::Doc
