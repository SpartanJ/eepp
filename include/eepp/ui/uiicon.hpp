#ifndef EE_UI_UIICON_HPP
#define EE_UI_UIICON_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/texture.hpp>

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

	/** Returns the icon source closest to the requested size.
	 *
	 * The returned drawable is shared icon data and must not be mutated by consumers. Use
	 * createDrawable() when a consumer needs its own mutable drawable instance. */
	virtual const DrawablePtr& getSource( const int& size ) const;

	/** Creates a private drawable instance for a consumer. This must not be called from a rendering
	 * loop; retain the returned instance instead. */
	DrawablePtr createDrawable( const int& size ) const;

	virtual void setSource( const int& size, DrawablePtr drawable );

  protected:
	UIIcon( const std::string& name );

	std::string mName;
	mutable UnorderedMap<int, DrawablePtr> mSizes;
};

class EE_API UIGlyphIcon : public UIIcon {
  public:
	static UIIcon* New( const std::string& name, FontTrueType* font, const Uint32& codePoint );

	virtual ~UIGlyphIcon();

	virtual const DrawablePtr& getSource( const int& size ) const;

  protected:
	UIGlyphIcon( const std::string& name, FontTrueType* font, const Uint32& codePoint );

	mutable FontTrueType* mFont;
	Uint32 mCodePoint;
	Uint32 mCloseCb{ 0 };
};

class EE_API UISVGIcon : public UIIcon {
  public:
	static UIIcon* New( const std::string& name, const std::string& svgXML );

	virtual ~UISVGIcon();

	virtual const DrawablePtr& getSource( const int& size ) const;

  protected:
	UISVGIcon( const std::string& name, const std::string& svgXML );

	std::string mSVGXml;
	mutable Sizei mOriSize;
	mutable int mOriChannels{ 0 };
};

}} // namespace EE::UI

#endif // EE_UI_UIICON_HPP
