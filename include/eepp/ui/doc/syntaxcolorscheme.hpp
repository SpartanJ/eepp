#ifndef EE_UI_DOC_SYNTAXSTYLE_HPP
#define EE_UI_DOC_SYNTAXSTYLE_HPP

#include <eepp/system/color.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>
#include <unordered_map>
#include <vector>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

/**
 * Syntax colors types accepted/used are:
 * "normal", "symbol", "comment", "keyword", "keyword2",
 * "number", "literal", "string", "operator", "function",
 * "link"
 *
 * Editor colors types accepted/used are:
 * "background", "text", "caret"
 * "selection", "line_number_background",
 * "line_number", "line_number2", "line_highlight",
 * "line_number_background", "whitespace", "line_break_column",
 * "matching_bracket", "matching_selection", "suggestion", "suggestion_selected"
 *
 * Following the lite editor syntax colors (https://github.com/rxi/lite).
 */
class EE_API SyntaxColorScheme {
  public:
	static SyntaxColorScheme getDefault();

	static std::vector<SyntaxColorScheme> loadFromStream( IOStream& stream );

	static std::vector<SyntaxColorScheme> loadFromFile( const std::string& path );

	static std::vector<SyntaxColorScheme> loadFromMemory( const void* data,
														  std::size_t sizeInBytes );

	static std::vector<SyntaxColorScheme> loadFromPack( Pack* pack, std::string filePackPath );

	struct Style {
		Style(){};
		Style( const Color& color ) : color( color ) {}
		Style( const Color& color, const Color& background, const Uint32& style ) :
			color( color ), background( background ), style( style ) {}
		Color color{Color::White};
		Color background{Color::Transparent};
		Uint32 style{0};
	};

	SyntaxColorScheme();

	SyntaxColorScheme( const std::string& name,
					   const std::unordered_map<std::string, Style>& syntaxColors,
					   const std::unordered_map<std::string, Style>& editorColors );

	const Style& getSyntaxStyle( const std::string& type ) const;

	void setSyntaxStyles( const std::unordered_map<std::string, Style>& styles );

	void setSyntaxStyle( const std::string& type, const Style& style );

	const Style& getEditorSyntaxStyle( const std::string& type ) const;

	const Color& getEditorColor( const std::string& type ) const;

	void setEditorSyntaxStyles( const std::unordered_map<std::string, Style>& styles );

	void setEditorSyntaxStyle( const std::string& type, const Style& style );

	const std::string& getName() const;

	void setName( const std::string& name );

  protected:
	std::string mName;
	std::unordered_map<std::string, Style> mSyntaxColors;
	std::unordered_map<std::string, Style> mEditorColors;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
