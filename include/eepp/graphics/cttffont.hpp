#ifndef EE_GRAPHICSCTTFFONT_H
#define EE_GRAPHICSCTTFFONT_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cfont.hpp>

namespace HaikuTTF {
	class hkFont;
}

namespace EE { namespace Graphics {

/** @brief This class loads True Type Font and then draw strings to the screen. */
class EE_API cTTFFont : public cFont {
	public:
		enum OutlineMethods
		{
			OutlineEntropia,	//! Slow, but better for small fonts
			OutlineFreetype		//! Faster, usually better for big fonts
		};

		//! Let the user select the default method to use for outlining the glyphs
		static OutlineMethods OutlineMethod;

		/** Creates an instance of a true type font */
		static cTTFFont * New( const std::string FontName );

		/** The destructor will not unload the texture from memory. If you want that you'll have to remove it manually ( cTextureFactory::instance()->Remove( MyFontInstance->GetTexId() ) ). */
		virtual ~cTTFFont();

		/** Loads a True Type Font from path
		* @param Filepath The TTF file path
		* @param Size The Size Width and Height for the font.
		* @param Style The Font Style
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @param AddPixelSeparator Indicates if separates the glyphs by a pixel to avoid problems with font scaling
		* @return If success
		*/
		bool Load( const std::string& Filepath, const unsigned int& Size, EE_TTF_FONT_STYLE Style = TTF_STYLE_NORMAL, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		/** Loads a True Type Font from pack
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		* @param Size The Size of the Font
		* @param Style The Font Style
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @param AddPixelSeparator Indicates if separates the glyphs by a pixel to avoid problems with font scaling
		* @return If success
		*/
		bool LoadFromPack( Pack* Pack, const std::string& FilePackPath, const unsigned int& Size, EE_TTF_FONT_STYLE Style = TTF_STYLE_NORMAL, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true  );

		/** Loads a True Type Font from memory
		* @param TTFData The pointer to the data
		* @param TTFDataSize The size of the data
		* @param Size The Size of the Font
		* @param Style The Font Style
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @param AddPixelSeparator Indicates if separates the glyphs by a pixel to avoid problems with font scaling
		* @return If success
		*/
		bool LoadFromMemory( Uint8* TTFData, const unsigned int& TTFDataSize, const unsigned int& Size, EE_TTF_FONT_STYLE Style = TTF_STYLE_NORMAL, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		/** Save the texture generated from the TTF file to disk */
		bool SaveTexture( const std::string& Filepath, const EE_SAVE_TYPE& Format = SAVE_TYPE_PNG );

		/** Save the characters coordinates to use it later to load the Texture Font */
		bool SaveCoordinates( const std::string& Filepath );

		/** Save the texture generated from the TTF file and the character coordinates. */
		bool Save( const std::string& TexturePath, const std::string& CoordinatesDatPath, const EE_SAVE_TYPE& Format = SAVE_TYPE_PNG );
	protected:
		friend class cTTFFontLoader;

		HaikuTTF::hkFont *	mFont;
		HaikuTTF::hkFont *	mFontOutline;
		eeColorA *	mPixels;

		std::string	mFilepath;
		Uint32		mNumChars;
		Uint8		mOutlineSize;

		eeColor		mFontColor;
		eeColor		mOutlineColor;

		EE_TTF_FONT_STYLE mStyle;

		Float		mTexWidth;
		Float		mTexHeight;

		bool		mLoadedFromMemory;
		bool		mThreadedLoading;
		bool		mTexReady;

		cTTFFont( const std::string FontName );

		bool ThreadedLoading() const;

		void ThreadedLoading( const bool& isThreaded );

		void UpdateLoading();

		bool iLoad( const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const eeColor& FontColor, Uint8 OutlineSize, const eeColor& OutlineColor, const bool& AddPixelSeparator );

		void MakeOutline( Uint8 *in, Uint8 *out, Int16 w, Int16 h, Int16 OutlineSize );

		void RebuildFromGlyphs();
};

}}

#endif
