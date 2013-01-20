#ifndef EE_GRAPHICSCTTFFONTLOADER
#define EE_GRAPHICSCTTFFONTLOADER

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cfont.hpp>
#include <eepp/graphics/cttffont.hpp>
#include <eepp/system/cobjectloader.hpp>

namespace EE { namespace Graphics {

/** @brief The TTF Font loader loads a true type font to memory in synchronous or asynchronous mode.
@see cObjectLoader
*/
class EE_API cTTFFontLoader : public cObjectLoader {
	public:
		/** Load a True Type Font from path
		* @param FontName The font name
		* @param Filepath The TTF file path
		* @param Size The Size Width and Height for the font.
		* @param Style The Font Style
		* @param VerticalDraw If draw in vertical instead of horizontal
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @param AddPixelSeparator Indicates if separates the glyphs by a pixel to avoid problems with font scaling
		*/
		cTTFFontLoader( const std::string& FontName, const std::string& Filepath, const eeUint& Size, EE_TTF_FONT_STYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		/** Load a True Type Font from a Pack
		* @param FontName The font name
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		* @param Size The Size of the Font
		* @param Style The Font Style
		* @param VerticalDraw If draw in vertical instead of horizontal
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @param AddPixelSeparator Indicates if separates the glyphs by a pixel to avoid problems with font scaling
		*/
		cTTFFontLoader( const std::string& FontName, cPack * Pack, const std::string& FilePackPath, const eeUint& Size, EE_TTF_FONT_STYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		/** Loads a True Type Font from memory
		* @param FontName The font name
		* @param TTFData The pointer to the data
		* @param TTFDataSize The size of the data
		* @param Size The Size of the Font
		* @param Style The Font Style
		* @param VerticalDraw If draw in vertical instead of horizontal
		* @param NumCharsToGen Determine the number of characters to generate ( from char 0 to ... x )
		* @param FontColor The Font color (this is the texture font color, if you plan to use a custom color and use outline, set it )
		* @param OutlineSize The Ouline Size
		* @param OutlineColor The Outline Color
		* @param AddPixelSeparator Indicates if separates the glyphs by a pixel to avoid problems with font scaling
		*/
		cTTFFontLoader( const std::string& FontName, Uint8* TTFData, const eeUint& TTFDataSize, const eeUint& Size, EE_TTF_FONT_STYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		virtual ~cTTFFontLoader();

		/** This must be called for the asynchronous mode to update the texture data to the GPU, the call must be done from the same thread that the GL Context was created ( the main thread ).
		**	The TTF Font creates texture from the data obtained from the true type file.
		** @see cObjectLoader::Update */
		void 				Update();

		/** Releases the Font instance and the texture loaded ( if was already loaded ), it will destroy the font texture from memory */
		void				Unload();

		/** @return The font name. */
		const std::string&	Id() const;

		/** @return The font instance if already exists, otherwise returns NULL. */
		cFont *				Font() const;
	protected:
		enum TTF_LOAD_TYPE
		{
			TTF_LT_PATH	= 1,
			TTF_LT_MEM	= 2,
			TTF_LT_PACK	= 3
		};

		Uint32				mLoadType; 	// From memory, from path, from pack

		cTTFFont *			mFont;

		std::string			mFontName;
		std::string			mFilepath;
		eeUint				mSize;
		EE_TTF_FONT_STYLE	mStyle;
		bool				mVerticalDraw;
		Uint16				mNumCharsToGen;
		eeColor				mFontColor;
		Uint8				mOutlineSize;
		eeColor				mOutlineColor;
		bool				mAddPixelSeparator;
		cPack *				mPack;
		Uint8 *				mData;
		eeUint				mDataSize;

		void 				Start();

		void				Reset();
	private:
		bool				mFontLoaded;

		void 				LoadFromPath();
		void				LoadFromMemory();
		void				LoadFromPack();
		void 				Create();
};

}}

#endif


