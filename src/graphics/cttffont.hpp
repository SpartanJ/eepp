#ifndef EE_GRAPHICSCTTFFONT_H
#define EE_GRAPHICSCTTFFONT_H

#include "base.hpp"
#include "ctexturefactory.hpp"
#include "cfont.hpp"

namespace EE { namespace Graphics {

/** @brief This class loads True Type Font and then draw strings to the screen. */
class EE_API cTTFFont : public cFont {
	public:
		cTTFFont();
		~cTTFFont();

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
		bool Load( const std::string& Filepath, const eeUint& Size, EE_TTF_FONTSTYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0) );
		
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
		bool LoadFromPack( cPack* Pack, const std::string& FilePackPath, const eeUint& Size, EE_TTF_FONTSTYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0)  );
		
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
		bool LoadFromMemory( Uint8* TTFData, const eeUint& TTFDataSize, const eeUint& Size, EE_TTF_FONTSTYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0) );

		/** Save the texture generated from the TTF file to disk */
		bool SaveTexture( const std::string& Filepath, const EE_SAVETYPE& Format = EE_SAVE_TYPE_TGA );

		/** Save the characters coordinates to use it later to load the Texture Font */
		bool SaveCoordinates( const std::string& Filepath );

		/** Save the texture generated from the TTF file and the character coordinates. */
		bool Save( const std::string& TexturePath, const std::string& CoordinatesDatPath, const EE_SAVETYPE& Format = EE_SAVE_TYPE_TGA );
	private:
		cTextureFactory* TF;
		TTF_Font* mFont;

		std::string mFilepath;
		Uint32 mBase;
		Uint32 mNumChars;
		Uint8 mOutlineSize;

		eeColor mFontColor, mOutlineColor;
		EE_TTF_FONTSTYLE mStyle;

		eeFloat mTexWidth, mTexHeight;
		bool mTTFInit;

		bool mLoadedFromMemory;

		bool iLoad( const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor );
		void MakeOutline( Uint8 *in, Uint8 *out, Int16 w, Int16 h);
		void RebuildFromGlyphs();
};

}}

#endif
