#ifndef EE_UI_DOC_SYNTAXSTYLE_HPP
#define EE_UI_DOC_SYNTAXSTYLE_HPP

#include <eepp/system/color.hpp>
#include <unordered_map>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class EE_API SyntaxColorScheme {
  public:
	static SyntaxColorScheme getDefault();

	SyntaxColorScheme();

	SyntaxColorScheme( const std::string& name,
					   const std::unordered_map<std::string, Color>& colors );

	const Color& getColor( const std::string& type ) const;

	void setColors( const std::unordered_map<std::string, Color>& colors );

	void setColor( const std::string& type, const Color& color );

	const std::string& getName() const;

	void setName( const std::string& name );

  protected:
	std::string mName;
	std::unordered_map<std::string, Color> mColors;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
