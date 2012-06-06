#ifndef EE_GRAPHICSCTTFFONT_H
#define EE_GRAPHICSCTTFFONT_H

#include "base.hpp"
#include "ctexturefactory.hpp"
#include "cfont.hpp"

namespace HaikuTTF {
	class hkFont;
}

namespace EE { namespace Graphics {

/** @brief This class loads True Type Font and then draw strings to the screen. */
class EE_API cTTFFont : public cFont {
	public:
		cTTFFont( const std::string FontName );

		/** The destructor will not unload the texture from memory. If you want that you'll have to remove it manually ( cTextureFactory::instance()->Remove( MyFontInstance->GetTexId() ) ). */
		virtual ~cTTFFont();

		/** Load a True Type Font from path
		* @param Filepath The TTF file path
		* @param Size The Size Width and Height for the font.
		* @param Style The Font Style
		* @param VerticalDraw If draw in vertical instead of horizontal
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @return If success
		*/
		bool Load( const std::string& Filepath, const eeUint& Size, EE_TTF_FONTSTYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		/** Load Font from pack
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		* @param Size The Size of the Font
		* @param Style The Font Style
		* @param VerticalDraw If draw in vertical instead of horizontal
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @return If success
		*/
		bool LoadFromPack( cPack* Pack, const std::string& FilePackPath, const eeUint& Size, EE_TTF_FONTSTYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true  );

		/** Load a True Type Font from memory
		* @param TTFData The pointer to the data
		* @param TTFDataSize The size of the data
		* @param Size The Size of the Font
		* @param Style The Font Style
		* @param VerticalDraw If draw in vertical instead of horizontal
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @return If success
		*/
		bool LoadFromMemory( Uint8* TTFData, const eeUint& TTFDataSize, const eeUint& Size, EE_TTF_FONTSTYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		/** Save the texture generated from the TTF file to disk */
		bool SaveTexture( const std::string& Filepath, const EE_SAVE_TYPE& Format = EE_SAVE_TYPE_PNG );

		/** Save the characters coordinates to use it later to load the Texture Font */
		bool SaveCoordinates( const std::string& Filepath );

		/** Save the texture generated from the TTF file and the character coordinates. */
		bool Save( const std::string& TexturePath, const std::string& CoordinatesDatPath, const EE_SAVE_TYPE& Format = EE_SAVE_TYPE_PNG );

	protected:
		friend class cTTFFontLoader;

		HaikuTTF::hkFont *	mFont;
		eeColorA *	mPixels;

		std::string	mFilepath;
		Uint32		mNumChars;
		Uint8		mOutlineSize;

		eeColor		mFontColor;
		eeColor		mOutlineColor;

		EE_TTF_FONTSTYLE mStyle;

		eeFloat		mTexWidth;
		eeFloat		mTexHeight;

		bool		mLoadedFromMemory;
		bool		mThreadedLoading;
		bool		mTexReady;

		bool ThreadedLoading() const;

		void ThreadedLoading( const bool& isThreaded );

		void UpdateLoading();

		bool iLoad( const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, Uint8 OutlineSize, const eeColor& OutlineColor, const bool& AddPixelSeparator );

		void MakeOutline( Uint8 *in, Uint8 *out, Int16 w, Int16 h);

		void RebuildFromGlyphs();
};

}}

#endif
