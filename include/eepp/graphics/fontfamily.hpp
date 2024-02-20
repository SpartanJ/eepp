#ifndef EE_GRAPHICS_FONTFAMILY_HPP
#define EE_GRAPHICS_FONTFAMILY_HPP

#include <eepp/graphics/fonttruetype.hpp>

namespace EE { namespace Graphics {

class EE_API FontFamily {
  public:
	static void loadFromRegular( FontTrueType* font, std::string overwriteFontName = "" );

  private:
	static std::string findType( const std::string& fontpath, const std::string& fontname,
								 const std::string& ext,
								 const std::vector<std::string_view>& names );

	static FontTrueType* setFont( FontTrueType* font, const std::string& fontpath,
								  const std::string_view& fontType );
};

}} // namespace EE::Graphics

#endif // FONTFAMILY_HPP
