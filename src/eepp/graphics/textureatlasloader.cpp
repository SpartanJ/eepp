#include <eepp/graphics/packerhelper.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/packmanager.hpp>

namespace EE { namespace Graphics {

using namespace Private;

TextureAtlasLoader* TextureAtlasLoader::New() {
	return eeNew( TextureAtlasLoader, () );
}

TextureAtlasLoader* TextureAtlasLoader::New( const std::string& TextureAtlasPath,
											 const bool& threaded, GLLoadCallback LoadCallback ) {
	return eeNew( TextureAtlasLoader, ( TextureAtlasPath, threaded, LoadCallback ) );
}

TextureAtlasLoader* TextureAtlasLoader::New( const Uint8* Data, const Uint32& DataSize,
											 const std::string& TextureAtlasName,
											 const bool& threaded, GLLoadCallback LoadCallback ) {
	return eeNew( TextureAtlasLoader,
				  ( Data, DataSize, TextureAtlasName, threaded, LoadCallback ) );
}

TextureAtlasLoader* TextureAtlasLoader::New( Pack* Pack, const std::string& FilePackPath,
											 const bool& threaded, GLLoadCallback LoadCallback ) {
	return eeNew( TextureAtlasLoader, ( Pack, FilePackPath, threaded, LoadCallback ) );
}

TextureAtlasLoader* TextureAtlasLoader::New( IOStream& IOS, const bool& threaded,
											 GLLoadCallback LoadCallback ) {
	return eeNew( TextureAtlasLoader, ( IOS, threaded, LoadCallback ) );
}

TextureAtlasLoader::TextureAtlasLoader() :
	mThreaded( false ),
	mLoaded( false ),
	mPack( NULL ),
	mSkipResourceLoad( false ),
	mIsLoading( false ),
	mTextureAtlas( NULL ) {}

TextureAtlasLoader::TextureAtlasLoader( const std::string& TextureAtlasPath, const bool& Threaded,
										GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( TextureAtlasPath ),
	mThreaded( Threaded ),
	mLoaded( false ),
	mPack( NULL ),
	mSkipResourceLoad( false ),
	mIsLoading( false ),
	mTextureAtlas( NULL ),
	mLoadCallback( LoadCallback ) {
	loadFromFile();
}

TextureAtlasLoader::TextureAtlasLoader( const Uint8* Data, const Uint32& DataSize,
										const std::string& TextureAtlasName, const bool& Threaded,
										GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( TextureAtlasName ),
	mThreaded( Threaded ),
	mLoaded( false ),
	mPack( NULL ),
	mSkipResourceLoad( false ),
	mIsLoading( false ),
	mTextureAtlas( NULL ),
	mLoadCallback( LoadCallback ) {
	loadFromMemory( Data, DataSize, TextureAtlasName );
}

TextureAtlasLoader::TextureAtlasLoader( Pack* Pack, const std::string& FilePackPath,
										const bool& Threaded, GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( FilePackPath ),
	mThreaded( Threaded ),
	mLoaded( false ),
	mPack( NULL ),
	mSkipResourceLoad( false ),
	mIsLoading( false ),
	mTextureAtlas( NULL ),
	mLoadCallback( LoadCallback ) {
	loadFromPack( Pack, FilePackPath );
}

TextureAtlasLoader::TextureAtlasLoader( IOStream& IOS, const bool& Threaded,
										GLLoadCallback LoadCallback ) :
	mThreaded( Threaded ),
	mLoaded( false ),
	mPack( NULL ),
	mSkipResourceLoad( false ),
	mIsLoading( false ),
	mTextureAtlas( NULL ),
	mLoadCallback( LoadCallback ) {
	loadFromStream( IOS );
}

TextureAtlasLoader::~TextureAtlasLoader() {}

void TextureAtlasLoader::setLoadCallback( GLLoadCallback LoadCallback ) {
	mLoadCallback = LoadCallback;
}

sTextureAtlasHdr TextureAtlasLoader::getTextureAtlasHeader() {
	return mTexGrHdr;
}

void TextureAtlasLoader::setTextureFilter( const Texture::TextureFilter& textureFilter ) {
	mTexGrHdr.TextureFilter = (char)textureFilter;

	size_t count = getTextureAtlas()->getTexturesCount() == 0;

	if ( count > 0 ) {
		for ( size_t i = 0; i < count; i++ )
			getTextureAtlas()->getTexture( i )->setFilter( textureFilter );
	}
}

void TextureAtlasLoader::loadFromStream( IOStream& IOS ) {
	mRL.setThreaded( mThreaded );

	if ( IOS.isOpen() ) {
		IOS.read( (char*)&mTexGrHdr, sizeof( sTextureAtlasHdr ) );

		if ( mTexGrHdr.Magic == EE_TEXTURE_ATLAS_MAGIC ) {
			for ( Uint32 i = 0; i < mTexGrHdr.TextureCount; i++ ) {
				sTextureHdr tTextureHdr;
				sTempTexAtlas tTexAtlas;

				IOS.read( (char*)&tTextureHdr, sizeof( sTextureHdr ) );

				tTexAtlas.Texture = tTextureHdr;
				tTexAtlas.TextureRegions.resize( tTextureHdr.TextureRegionCount );

				std::string name( &tTextureHdr.Name[0] );
				std::string path( FileSystem::fileRemoveFileName( mTextureAtlasPath ) + name );

				//! Checks if the texture is already loaded
				Texture* tTex = TextureFactory::instance()->getByName( path );

				if ( !mSkipResourceLoad && NULL == tTex ) {
					if ( NULL != mPack ) {
						mRL.add( [=] { TextureFactory::instance()->loadFromPack( mPack, path ); } );
					} else {
						mRL.add( [=] { TextureFactory::instance()->loadFromFile( path ); } );
					}
				}

				IOS.read( (char*)&tTexAtlas.TextureRegions[0],
						  sizeof( sTextureRegionHdr ) * tTextureHdr.TextureRegionCount );

				mTempAtlass.push_back( tTexAtlas );
			}
		}

		if ( !mSkipResourceLoad ) {
			mIsLoading = true;
			mRL.load( [&]( ResourceLoader* ) {
				if ( !mLoaded ) {
					createTextureRegions();
				}
			} );
		}
	}
}

void TextureAtlasLoader::loadFromFile( const std::string& TextureAtlasPath ) {
	if ( TextureAtlasPath.size() )
		mTextureAtlasPath = TextureAtlasPath;

	if ( FileSystem::fileExists( mTextureAtlasPath ) ) {
		IOStreamFile IOS( mTextureAtlasPath );

		loadFromStream( IOS );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tgPath( mTextureAtlasPath );

		Pack* tPack = PackManager::instance()->exists( tgPath );

		if ( NULL != tPack ) {
			loadFromPack( tPack, tgPath );
		}
	}
}

void TextureAtlasLoader::loadFromPack( Pack* Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( FilePackPath ) ) {
		mPack = Pack;

		ScopedBuffer buffer;

		Pack->extractFileToMemory( FilePackPath, buffer );

		loadFromMemory( buffer.get(), buffer.length(), FilePackPath );
	}
}

void TextureAtlasLoader::loadFromMemory( const Uint8* Data, const Uint32& DataSize,
										 const std::string& TextureAtlasName ) {
	if ( TextureAtlasName.size() )
		mTextureAtlasPath = TextureAtlasName;

	IOStreamMemory IOS( (const char*)Data, DataSize );

	loadFromStream( IOS );
}

TextureAtlas* TextureAtlasLoader::getTextureAtlas() const {
	return mTextureAtlas;
}

void TextureAtlasLoader::createTextureRegions() {
	mIsLoading = false;
	bool IsAlreadyLoaded = false;

	for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
		sTempTexAtlas* tTexAtlas = &mTempAtlass[z];
		sTextureHdr* tTexHdr = &tTexAtlas->Texture;

		std::string name( &tTexHdr->Name[0] );
		std::string path( FileSystem::fileRemoveFileName( mTextureAtlasPath ) + name );

		FileSystem::filePathRemoveProcessPath( path );

		Texture* tTex = TextureFactory::instance()->getByName( path );

		if ( NULL != tTex )
			mTexturesLoaded.push_back( tTex );

		// Create the Texture Atlas with the name of the real texture, not the Childs ( example
		// load 1.png and not 1_ch1.png )
		if ( 0 == z ) {
			std::string etapath =
				FileSystem::fileRemoveExtension( path ) + EE_TEXTURE_ATLAS_EXTENSION;

			TextureAtlas* tTextureAtlas = TextureAtlasManager::instance()->getByName( name );

			if ( NULL != tTextureAtlas && tTextureAtlas->getPath() == etapath ) {
				mTextureAtlas = tTextureAtlas;

				IsAlreadyLoaded = true;
			} else {
				mTextureAtlas = TextureAtlas::New( name );

				mTextureAtlas->setPath( etapath );

				TextureAtlasManager::instance()->add( mTextureAtlas );
			}
		}

		if ( NULL != tTex ) {
			if ( !IsAlreadyLoaded ) {
				for ( Int32 i = 0; i < tTexHdr->TextureRegionCount; i++ ) {
					sTextureRegionHdr* tSh = &tTexAtlas->TextureRegions[i];

					std::string TextureRegionName( &tSh->Name[0] );
					Rect tRect( tSh->X, tSh->Y, tSh->X + tSh->Width, tSh->Y + tSh->Height );

					TextureRegion* tTextureRegion = TextureRegion::New(
						tTex->getTextureId(), tRect,
						Sizef( (Float)tSh->DestWidth, (Float)tSh->DestHeight ),
						Vector2i( tSh->OffsetX, tSh->OffsetY ), TextureRegionName );

					tTextureRegion->setPixelDensity( PixelDensity::toFloat( tSh->PixelDensity ) );
					// if ( tSh->Flags & HDR_TEXTUREREGION_FLAG_FLIPED )
					// Should rotate the sub texture, but.. sub texture rotation is not stored.

					mTextureAtlas->add( tTextureRegion );
				}
			}
		} else {
			eePRINTL( "TextureAtlasLoader::createTextureRegions: Failed to find texture atlas "
					  "texture, it seems that is not loaded for some reason. Couldn't find: %s",
					  path.c_str() );

			eeASSERT( NULL != tTex );

			return;
		}
	}

	if ( NULL != mTextureAtlas && mTexturesLoaded.size() ) {
		for ( size_t i = 0; i < mTexturesLoaded.size(); i++ )
			mTexturesLoaded[i]->setFilter( (Texture::TextureFilter)mTexGrHdr.TextureFilter );

		mTextureAtlas->setTextures( mTexturesLoaded );
	}

	mLoaded = true;

	if ( mLoadCallback ) {
		mLoadCallback( this );
	}
}

bool TextureAtlasLoader::isThreaded() const {
	return mThreaded;
}

void TextureAtlasLoader::setThreaded( const bool& threaded ) {
	mThreaded = threaded;
}

const bool& TextureAtlasLoader::isLoaded() const {
	return mLoaded;
}

const bool& TextureAtlasLoader::isLoading() const {
	return mIsLoading;
}

Texture* TextureAtlasLoader::getTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTexturesLoaded.size() );
	return mTexturesLoaded[texnum];
}

Uint32 TextureAtlasLoader::getTexturesLoadedCount() {
	return mTexturesLoaded.size();
}

bool TextureAtlasLoader::updateTextureAtlas() {
	if ( NULL == mTextureAtlas || !mTextureAtlasPath.size() )
		return false;

	//! Update the data of the texture atlas
	for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
		sTempTexAtlas* tTexAtlas = &mTempAtlass[z];
		sTextureHdr* tTexHdr = &tTexAtlas->Texture;

		for ( Int32 i = 0; i < tTexHdr->TextureRegionCount; i++ ) {
			sTextureRegionHdr* tSh = &tTexAtlas->TextureRegions[i];
			TextureRegion* tTextureRegion = mTextureAtlas->getById( tSh->ResourceID );

			if ( NULL != tTextureRegion ) {
				tSh->OffsetX = tTextureRegion->getOffset().x;
				tSh->OffsetY = tTextureRegion->getOffset().y;
				tSh->DestWidth = (Int32)tTextureRegion->getDestSize().x;
				tSh->DestHeight = (Int32)tTextureRegion->getDestSize().y;
			}
		}
	}

	IOStreamFile fs( mTextureAtlasPath, "wb" );

	if ( fs.isOpen() ) {
		fs.write( reinterpret_cast<char*>( &mTexGrHdr ), sizeof( sTextureAtlasHdr ) );

		for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
			sTempTexAtlas* tTexAtlas = &mTempAtlass[z];
			sTextureHdr* tTexHdr = &tTexAtlas->Texture;

			fs.write( reinterpret_cast<char*>( tTexHdr ), sizeof( sTextureHdr ) );

			fs.write( reinterpret_cast<char*>( &tTexAtlas->TextureRegions[0] ),
					  sizeof( sTextureRegionHdr ) *
						  (std::streamsize)tTexAtlas->TextureRegions.size() );
		}

		return true;
	}

	return false;
}

bool TextureAtlasLoader::updateTextureAtlas( std::string TextureAtlasPath, std::string ImagesPath,
											 Sizei maxImageSize ) {
	if ( !TextureAtlasPath.size() || !ImagesPath.size() ||
		 !FileSystem::fileExists( TextureAtlasPath ) || !FileSystem::isDirectory( ImagesPath ) )
		return false;

	mSkipResourceLoad = true;
	loadFromFile( TextureAtlasPath );
	mSkipResourceLoad = false;

	if ( !mTempAtlass.size() )
		return false;

	Int32 x, y, c;

	Int32 NeedUpdate = 0;
	PixelDensitySize pixelDensity = PixelDensitySize::MDPI;

	FileSystem::dirPathAddSlashAtEnd( ImagesPath );

	Uint32 z;

	Uint32 totalTextureRegions = 0;
	for ( z = 0; z < mTempAtlass.size(); z++ ) {
		totalTextureRegions += mTempAtlass[z].Texture.TextureRegionCount;

		if ( mTempAtlass[z].Texture.TextureRegionCount > 0 ) {
			pixelDensity = (PixelDensitySize)mTempAtlass[z].TextureRegions[0].PixelDensity;
		}
	}

	Uint32 totalImages = 0;
	std::vector<std::string> PathFiles = FileSystem::filesGetInPath( ImagesPath );

	for ( z = 0; z < PathFiles.size(); z++ ) {
		std::string realpath( ImagesPath + PathFiles[z] );

		// Avoids reading file headers for known extensions
		if ( Image::isImageExtension( realpath ) )
			totalImages++;
	}

	if ( totalTextureRegions != totalImages ) {
		NeedUpdate = 2;
	} else {
		for ( z = 0; z < mTempAtlass.size(); z++ ) {
			sTempTexAtlas* tTexAtlas = &mTempAtlass[z];
			sTextureHdr* tTexHdr = &tTexAtlas->Texture;

			if ( 2 != NeedUpdate ) {
				for ( Int32 i = 0; i < tTexHdr->TextureRegionCount; i++ ) {
					sTextureRegionHdr* tSh = &tTexAtlas->TextureRegions[i];

					std::string path( ImagesPath + tSh->Name );

					if ( FileSystem::fileSize( path ) ) {
						if ( tSh->Date != FileSystem::fileGetModificationDate( path ) ) {
							if ( Image::getInfo( path.c_str(), &x, &y, &c ) ) {
								if ( ( !( tSh->Flags & HDR_TEXTUREREGION_FLAG_FLIPED ) &&
									   tSh->Width == x &&
									   tSh->Height == y ) || // If size or channels changed, the
															 // image need update
									 ( ( tSh->Flags & HDR_TEXTUREREGION_FLAG_FLIPED ) &&
									   tSh->Width == y && tSh->Height == x ) ||
									 tSh->Channels != c ) {
									NeedUpdate = 1; // Only update the image with the newest one
								} else {
									NeedUpdate = 2; // The image change it, recreate all
									break;
								}
							} else {
								NeedUpdate = 2; // Something is wrong on the image
								break;
							}
						}
					} else {
						NeedUpdate = 2; // Need recreation of the whole texture atlas, some image
										// where deleted.
						break;
					}
				}
			} else {
				break;
			}
		}
	}

	if ( NeedUpdate ) {
		std::string tapath( FileSystem::fileRemoveExtension( TextureAtlasPath ) + "." +
							Image::saveTypeToExtension( mTexGrHdr.Format ) );

		if ( 2 == NeedUpdate ) {
			TexturePacker tp(
				maxImageSize.getWidth() == 0 ? mTexGrHdr.Width : maxImageSize.getWidth(),
				maxImageSize.getHeight() == 0 ? mTexGrHdr.Height : maxImageSize.getHeight(),
				pixelDensity, 0 != ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_POW_OF_TWO ),
				0 != ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_SCALABLE_SVG ), mTexGrHdr.PixelBorder,
				(Texture::TextureFilter)mTexGrHdr.TextureFilter,
				mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_ALLOW_FLIPPING );

			tp.addTexturesPath( ImagesPath );

			if ( tp.packTextures() <= 0 ) {
				return false;
			}

			tp.save( tapath, (Image::SaveType)mTexGrHdr.Format );
		} else if ( 1 == NeedUpdate ) {
			std::string etapath =
				FileSystem::fileRemoveExtension( tapath ) + EE_TEXTURE_ATLAS_EXTENSION;

			IOStreamFile fs( etapath, "wb" );

			if ( !fs.isOpen() )
				return false;

			fs.write( reinterpret_cast<const char*>( &mTexGrHdr ), sizeof( sTextureAtlasHdr ) );

			for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
				if ( z != 0 ) {
					tapath = FileSystem::fileRemoveExtension( TextureAtlasPath ) + "_ch" +
							 String::toStr( z ) + "." +
							 Image::saveTypeToExtension( mTexGrHdr.Format );
				}

				Image Img( tapath );

				if ( NULL != Img.getPixelsPtr() ) {
					sTempTexAtlas* tTexAtlas = &mTempAtlass[z];
					sTextureHdr* tTexHdr = &tTexAtlas->Texture;

					fs.write( reinterpret_cast<const char*>( tTexHdr ), sizeof( sTextureHdr ) );

					for ( Int32 i = 0; i < tTexHdr->TextureRegionCount; i++ ) {
						sTextureRegionHdr* tSh = &tTexAtlas->TextureRegions[i];

						std::string imgcopypath( ImagesPath + tSh->Name );

						Uint32 ModifDate = FileSystem::fileGetModificationDate( imgcopypath );

						if ( tSh->Date != ModifDate ) {
							tSh->Date = ModifDate; // Update the sub texture hdr

							Image ImgCopy( imgcopypath );

							if ( NULL != ImgCopy.getPixelsPtr() ) {
								Img.copyImage( &ImgCopy, tSh->X,
											   tSh->Y ); // Update the image into the texture atlas
							} else {
								break;
							}
						}
					}

					fs.write( reinterpret_cast<const char*>( &tTexAtlas->TextureRegions[0] ),
							  sizeof( sTextureRegionHdr ) * tTexHdr->TextureRegionCount );

					Img.saveToFile( tapath, (Image::SaveType)mTexGrHdr.Format );
				} else {
					return false; // fatal error
				}
			}
		}
	}

	return true;
}

}} // namespace EE::Graphics
