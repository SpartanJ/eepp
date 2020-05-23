#include <eepp/ui/doc/syntaxstyle.hpp>

namespace EE { namespace UI { namespace Doc {

SyntaxStyle SyntaxStyle::getDefault() {
	SyntaxStyle style;
	std::unordered_map<std::string, Color> colors;
	colors["normal"] = Color( "#e1e1e6" );
	colors["symbol"] = Color( "#e1e1e6" );
	colors["comment"] = Color( "#676b6f" );
	colors["keyword"] = Color( "#E58AC9" );
	colors["keyword2"] = Color( "#F77483" );
	colors["number"] = Color( "#FFA94D" );
	colors["literal"] = Color( "#FFA94D" );
	colors["string"] = Color( "#f7c95c" );
	colors["operator"] = Color( "#93DDFA" );
	colors["function"] = Color( "#93DDFA" );
	style.setColors( colors );
	return style;
}

SyntaxStyle::SyntaxStyle() {}

const Color& SyntaxStyle::getColor( const std::string& type ) const {
	auto it = mColors.find( type );
	if ( it != mColors.end() )
		return it->second;
	return Color::White;
}

void SyntaxStyle::setColors( const std::unordered_map<std::string, Color>& colors ) {
	mColors.insert( colors.begin(), colors.end() );
}

void SyntaxStyle::setColor( const std::string& type, const Color& color ) {
	mColors[type] = color;
}

}}} // namespace EE::UI::Doc
