#ifndef HAIKUTTF_HKFONTMANAGER_HPP
#define HAIKUTTF_HKFONTMANAGER_HPP

#include "hkbase.hpp"
#include "hkglyph.hpp"
#include "hkfont.hpp"
#include "hkmutex.hpp"

namespace HaikuTTF {

class hkFontManager : private hkMutex {
	static hkFontManager * mSingleton;
	public:
		static hkFontManager * exists();

		static hkFontManager * instance();

		static void destroySingleton();

		hkFontManager();

		~hkFontManager();

		int 				init();

		void 				destroy();

		int 				wasInit();

		void 				closeFont( hkFont * Font );

		hkFont * 			openFromMemory( const u8* data, unsigned long size, int ptsize, long index = 0, unsigned int glyphCacheSize = 256 );

		hkFont * 			openFromFile( const char* filename, int ptsize, long index = 0, unsigned int glyphCacheSize = 256 );

		FT_Library 			getLibrary() const;
	protected:
		friend class hkFont;
		
		FT_Library 			mLibrary;
		int 				mInitialized;

		hkFont * 			fontPrepare( hkFont * font, int ptsize );
		
		void mutexLock();
		
		void mutexUnlock();
};

}

#endif
