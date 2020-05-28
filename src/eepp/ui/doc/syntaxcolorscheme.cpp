#include <eepp/ui/doc/syntaxcolorscheme.hpp>

namespace EE { namespace UI { namespace Doc {

// Color schemes are compatible with the lite (https://github.com/rxi/lite) color schemes.

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
			},
			{
				{"background", Color( "#2e2e32" )},
				{"text", Color( "#e1e1e6" )},
				{"caret", Color( "#93DDFA" )},
				{"selection", Color( "#48484f" )},
				{"line_highlight", Color( "#343438" )},
				{"line_number", Color( "#525259" )},
				{"line_number2", Color( "#83838f" )},
				{"line_number_background", Color::Transparent},
			}};
}

SyntaxColorScheme::SyntaxColorScheme() {}

SyntaxColorScheme::SyntaxColorScheme( const std::string& name,
									  const std::unordered_map<std::string, Color>& syntaxColors,
									  const std::unordered_map<std::string, Color>& editorColors ) :
	mName( name ), mSyntaxColors( syntaxColors ), mEditorColors( editorColors ) {}

const Color& SyntaxColorScheme::getSyntaxColor( const std::string& type ) const {
	auto it = mSyntaxColors.find( type );
	if ( it != mSyntaxColors.end() )
		return it->second;
	return Color::White;
}

void SyntaxColorScheme::setSyntaxColors( const std::unordered_map<std::string, Color>& colors ) {
	mSyntaxColors.insert( colors.begin(), colors.end() );
}

void SyntaxColorScheme::setSyntaxColor( const std::string& type, const Color& color ) {
	mSyntaxColors[type] = color;
}

const Color& SyntaxColorScheme::getEditorColor( const std::string& type ) const {
	auto it = mEditorColors.find( type );
	if ( it != mEditorColors.end() )
		return it->second;
	return Color::White;
}

void SyntaxColorScheme::setEditorColors( const std::unordered_map<std::string, Color>& colors ) {
	mEditorColors.insert( colors.begin(), colors.end() );
}

void SyntaxColorScheme::setEditorColor( const std::string& type, const Color& color ) {
	mEditorColors[type] = color;
}

const std::string& SyntaxColorScheme::getName() const {
	return mName;
}

void SyntaxColorScheme::setName( const std::string& name ) {
	mName = name;
}

}}} // namespace EE::UI::Doc
