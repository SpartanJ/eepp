#ifndef EE_UI_UIICON_HPP
#define EE_UI_UIICON_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {
class FontTrueType;
}} // namespace EE::Graphics
using namespace EE::Graphics;

namespace EE { namespace UI {

class EE_API UIIcon {
  public:
	static UIIcon* New( const std::string& name );

	virtual ~UIIcon();

	const std::string& getName() const;

	virtual Drawable* getSize( const int& size ) const;

	virtual void setSize( const int& size, Drawable* drawable );

  protected:
	UIIcon( const std::string& name );

	std::string mName;
	mutable std::map<int, Drawable*> mSizes;
};

class EE_API UIGlyphIcon : public UIIcon {
  public:
	static UIIcon* New( const std::string& name, FontTrueType* font, const Uint32& codePoint );

	virtual Drawable* getSize( const int& size ) const;

  protected:
	UIGlyphIcon( const std::string& name, FontTrueType* font, const Uint32& codePoint );

	mutable FontTrueType* mFont;
	Uint32 mCodePoint;
};

}} // namespace EE::UI

#endif // EE_UI_UIICON_HPP
