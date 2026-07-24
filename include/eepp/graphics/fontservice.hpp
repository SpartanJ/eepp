#ifndef EE_GRAPHICS_FONTSERVICE_HPP
#define EE_GRAPHICS_FONTSERVICE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace Graphics {

class FontTrueType;
class ResourceScope;
struct FontDesc;

/**
 * @brief Owns font fallback state and rendering policy for a ResourceScope.
 *
 * Every ResourceScope has one FontService. Fonts published locally by that scope are associated
 * with its service, allowing glyph lookup to resolve configured emoji, explicit fallback, and
 * system fallback fonts without consulting global state.
 *
 * The service strongly owns configured emoji and fallback fonts. Raw-pointer setters only accept
 * fonts that can be resolved through the associated scope; passing an unrelated pointer clears or
 * ignores the corresponding configuration. Removing a locally published font from the scope also
 * removes any service references to it.
 *
 * Rendering policy changes are applied to TrueType fonts associated with this service. Imported
 * fonts remain associated with the service of their owning scope and are therefore not mutated.
 *
 * System fonts can be loaded with two different lifetime contracts:
 * - loadSystemFont() returns an independently owned, uncached font.
 * - getOrLoadSystemFallbackFont() retains the font in this service for repeated fallback lookup.
 */
class EE_API FontService {
	friend class ResourceScope;

  public:
	/** Creates a service associated with @p resourceScope. ResourceScope owns its service. */
	explicit FontService( ResourceScope& resourceScope );

	/** @return The resource scope whose fonts and policy are managed by this service. */
	ResourceScope& getResourceScope() const;

	/** @return The configured color emoji font, or nullptr when none is configured. */
	Font* getColorEmojiFont() const;

	/**
	 * Sets the color emoji font. The font must be resolvable through the associated scope. Passing
	 * nullptr or an unrelated font clears the current value.
	 */
	void setColorEmojiFont( Font* font );

	/** @return The configured monochrome emoji font, or nullptr when none is configured. */
	Font* getEmojiFont() const;

	/**
	 * Sets the monochrome emoji font. The font must be resolvable through the associated scope.
	 * Passing nullptr or an unrelated font clears the current value.
	 */
	void setEmojiFont( Font* font );

	/** @return The strongly owned explicit fallback fonts in lookup order. */
	const std::vector<FontPtr>& getFallbackFonts() const;

	/** @return Whether at least one explicit fallback font is configured. */
	bool hasFallbackFonts() const;

	/** Adds an owning fallback handle unless the same font is already configured. */
	bool addFallbackFont( FontPtr fallbackFont );

	/**
	 * Adds a fallback font resolved through the associated scope.
	 * @return True when the font was found and added; false otherwise.
	 */
	bool addFallbackFont( Font* fallbackFont );

	/** Removes the matching explicit fallback font without removing it from its resource scope. */
	bool removeFallbackFont( Font* fallbackFont );

	/** @return The policy used when loading and updating associated TrueType fonts. */
	FontHinting getHinting() const;

	/** Updates the hinting policy and applies it to associated non-emoji TrueType fonts. */
	void setHinting( FontHinting hinting );

	/** @return The antialiasing policy used by associated TrueType fonts. */
	FontAntialiasing getAntialiasing() const;

	/** Updates the antialiasing policy and applies it to associated non-emoji TrueType fonts. */
	void setAntialiasing( FontAntialiasing antialiasing );

	/**
	 * Finds a TrueType font visible through the associated scope by its runtime internal ID.
	 * @return A borrowed pointer, or nullptr when no matching font is visible.
	 */
	Font* getByInternalId( Uint32 internalId ) const;

	/**
	 * Loads a standalone system font described by @p desc.
	 *
	 * The font is not published into the associated scope and is not retained by this service. The
	 * returned handle is its sole owner and can be released independently, immediately freeing its
	 * glyph pages when no other handle exists. Current hinting and antialiasing policy is applied
	 * at load time. The standalone font does not use this service for subsequent fallback
	 * resolution.
	 *
	 * @return An owning handle, or an empty handle when the descriptor cannot be loaded.
	 */
	ResourcePtr<FontTrueType> loadSystemFont( const FontDesc& desc );

	/**
	 * Finds or loads a system fallback font described by @p desc.
	 *
	 * Successfully loaded fonts are published into the associated scope and strongly retained by
	 * the service for future glyph fallback requests. The returned pointer is borrowed and remains
	 * valid until the font is removed from the scope or the service is destroyed.
	 *
	 * @return A borrowed pointer to the cached font, or nullptr when loading fails.
	 */
	FontTrueType* getOrLoadSystemFallbackFont( const FontDesc& desc );

  private:
	FontPtr findHandle( Font* font ) const;
	void onFontRemoved( Font* font );

	ResourceScope& mResourceScope;
	FontPtr mColorEmojiFont;
	FontPtr mEmojiFont;
	std::vector<FontPtr> mFallbackFonts;
	std::vector<FontPtr> mSystemFallbackFonts;
	FontHinting mHinting{ FontHinting::Full };
	FontAntialiasing mAntialiasing{ FontAntialiasing::Grayscale };
};

}} // namespace EE::Graphics

#endif
