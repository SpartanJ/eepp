#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/font.hpp>

#include <eepp/system/resourcemanager.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The Font Manager is a singleton class that manages all the instance of fonts
   instanciated. And releases the font instances automatically. So the user doesn't need to release
   any font instance.
*/
class EE_API FontManager : public ResourceManager<Font> {
	SINGLETON_DECLARE_HEADERS( FontManager )

  public:
	virtual ~FontManager();

	/** @brief Adds a new font to the manager */
	Graphics::Font* add( Graphics::Font* Font );

	Font* getColorEmojiFont() const;

	void setColorEmojiFont( Graphics::Font* font );

	Font* getEmojiFont() const;

	void setEmojiFont( Font* newEmojiFont );

	Font* getFallbackFont() const;

	void setFallbackFont( Font* fallbackFont );

  protected:
	Font* mColorEmojiFont{ nullptr };
	Font* mEmojiFont{ nullptr };
	Font* mFallbackFont{ nullptr };
	FontManager();
};

}} // namespace EE::Graphics

#endif
