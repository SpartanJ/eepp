#ifndef EE_GRAPHICSCTEXTUREFONTLOADER
#define EE_GRAPHICSCTEXTUREFONTLOADER

#include "base.hpp"
#include "cfont.hpp"
#include "ctexturefont.hpp"
#include "../system/cobjectloader.hpp"
#include "ctextureloader.hpp"

namespace EE { namespace Graphics {

#define TEF_LT_PATH 	(1)
#define TEF_LT_MEM 		(2)
#define TEF_LT_PACK 	(3)
#define TEF_LT_TEX		(4)

class EE_API cTextureFontLoader : public cObjectLoader {
	public:
		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const eeUint& StartChar = 0, const eeUint& Spacing = 0, const bool& VerticalDraw = false, const eeUint& TexColumns = 16, const eeUint& TexRows = 16, const Uint16& NumChars = 256 );

		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const std::string& CoordinatesDatPath, const bool& VerticalDraw = false );

		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, cPack * Pack, const std::string& FilePackPath, const bool& VerticalDraw = false );

		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const Uint8* CoordData, const Uint32& CoordDataSize, const bool& VerticalDraw = false );

		virtual ~cTextureFontLoader();

		void 				Update();

		const std::string& Id() const;

		cFont *				Font() const;
	protected:
		Uint32				mLoadType; 	// From memory, from path, from pack

		cTextureFont *	mFont;

		std::string			mFontName;
		cTextureLoader *	mTexLoader;

		std::string			mFilepath;

		eeUint				mStartChar;
		eeUint				mSpacing;
		bool				mVerticalDraw;
		eeUint				mTexColumns;
		eeUint				mTexRows;
		eeUint				mNumChars;

		cPack *				mPack;

		const Uint8 *		mData;
		Uint32				mDataSize;

		void 				Start();
	private:
		bool				mTexLoaded;
		bool				mFontLoaded;

		void				LoadFont();
		void 				LoadFromPath();
		void				LoadFromMemory();
		void				LoadFromPack();
		void				LoadFromTex();
};

}}

#endif


