#ifndef EE_UI_DOC_SYNTAXSTYLE_HPP
#define EE_UI_DOC_SYNTAXSTYLE_HPP

#include <eepp/system/color.hpp>
#include <unordered_map>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

/**
 * Syntax colors types accepted/used are:
 * "normal", "symbol", "comment", "keyword", "keyword2",
 * "number", "literal", "string", "operator", "function"
 *
 * Editor colors types accepted/used are:
 * "background", "text", "caret"
 * "selection", "line_number_background",
 * "line_number", "line_number2", "line_highlight"
 *
 * Following the lite editor syntax colors (https://github.com/rxi/lite).
 */
class EE_API SyntaxColorScheme {
  public:
	static SyntaxColorScheme getDefault();

	SyntaxColorScheme();

	SyntaxColorScheme( const std::string& name,
					   const std::unordered_map<std::string, Color>& syntaxColors,
					   const std::unordered_map<std::string, Color>& editorColors );

	const Color& getSyntaxColor( const std::string& type ) const;

	void setSyntaxColors( const std::unordered_map<std::string, Color>& colors );

	void setSyntaxColor( const std::string& type, const Color& color );

	const Color& getEditorColor( const std::string& type ) const;

	void setEditorColors( const std::unordered_map<std::string, Color>& colors );

	void setEditorColor( const std::string& type, const Color& color );

	const std::string& getName() const;

	void setName( const std::string& name );

  protected:
	std::string mName;
	std::unordered_map<std::string, Color> mSyntaxColors;
	std::unordered_map<std::string, Color> mEditorColors;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
