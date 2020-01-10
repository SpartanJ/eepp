#include <algorithm>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/graphics/texturepackernode.hpp>
#include <eepp/graphics/texturepackertex.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/sys.hpp>

namespace EE { namespace Graphics {

TexturePacker* TexturePacker::New() {
	return eeNew( TexturePacker, () );
}

TexturePacker* TexturePacker::New( const Uint32& MaxWidth, const Uint32& MaxHeight,
								   const PixelDensitySize& PixelDensity, const bool& ForcePowOfTwo,
								   const bool& scalableSVG, const Uint32& PixelBorder,
								   const Texture::TextureFilter& textureFilter,
								   const bool& AllowFlipping ) {
	return eeNew( TexturePacker, ( MaxWidth, MaxHeight, PixelDensity, ForcePowOfTwo, scalableSVG,
								   PixelBorder, textureFilter, AllowFlipping ) );
}

TexturePacker::TexturePacker( const Uint32& MaxWidth, const Uint32& MaxHeight,
							  const PixelDensitySize& PixelDensity, const bool& ForcePowOfTwo,
							  const bool& scalableSVG, const Uint32& PixelBorder,
							  const Texture::TextureFilter& textureFilter,
							  const bool& AllowFlipping ) :
	mTotalArea( 0 ),
	mFreeList( NULL ),
	mWidth( 128 ),
	mHeight( 128 ),
	mPacked( false ),
	mAllowFlipping( false ),
	mChild( NULL ),
	mStrategy( PackBig ),
	mCount( 0 ),
	mParent( NULL ),
	mPlacedCount( 0 ),
	mForcePowOfTwo( true ),
	mPixelBorder( 0 ),
	mPixelDensity( PixelDensity ),
	mTextureFilter( textureFilter ),
	mSaveExtensions( false ),
	mScalableSVG( scalableSVG ),
	mFormat( Image::SaveType::SAVE_TYPE_PNG ) {
	setOptions( MaxWidth, MaxHeight, PixelDensity, ForcePowOfTwo, scalableSVG, PixelBorder,
				textureFilter, AllowFlipping );
}

TexturePacker::TexturePacker() :
	mTotalArea( 0 ),
	mFreeList( NULL ),
	mWidth( 128 ),
	mHeight( 128 ),
	mMaxSize( mWidth, mHeight ),
	mPacked( false ),
	mAllowFlipping( false ),
	mChild( NULL ),
	mStrategy( PackBig ),
	mCount( 0 ),
	mParent( NULL ),
	mPlacedCount( 0 ),
	mForcePowOfTwo( true ),
	mPixelBorder( 0 ),
	mPixelDensity( PixelDensitySize::MDPI ),
	mTextureFilter( Texture::TextureFilter::Linear ),
	mSaveExtensions( false ),
	mScalableSVG( false ),
	mFormat( Image::SaveType::SAVE_TYPE_PNG ) {}

TexturePacker::~TexturePacker() {
	close();
}

void TexturePacker::close() {
	reset();

	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		eeSAFE_DELETE( ( *it ) );
	}

	mTextures.clear();
}

void TexturePacker::reset() {
	mStrategy = PackBig;
	mCount = mTextures.size();

	TexturePackerTex* t = NULL;
	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		t = ( *it );
		t->placed( false );
	}

	if ( NULL != mFreeList ) {
		TexturePackerNode* next = mFreeList;
		TexturePackerNode* kill = NULL;

		while ( NULL != next ) {
			kill = next;

			next = next->getNext();

			eeSAFE_DELETE( kill );
		}

		mFreeList = NULL;
	}

	eeSAFE_DELETE( mChild );
}

Uint32 TexturePacker::getAtlasNumChannels() {
	Uint32 maxChannels = 0;
	TexturePackerTex* t = NULL;
	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		t = ( *it );

		if ( t->placed() ) {
			maxChannels = eemax( maxChannels, (Uint32)t->channels() );
		}
	}

	return maxChannels;
}

void TexturePacker::setOptions( const Uint32& MaxWidth, const Uint32& MaxHeight,
								const PixelDensitySize& PixelDensity, const bool& ForcePowOfTwo,
								const bool& scalableSVG, const Uint32& PixelBorder,
								const Texture::TextureFilter& textureFilter,
								const bool& AllowFlipping ) {
	if ( !mTextures.size() ) { // only can change the dimensions before adding any texture
		mMaxSize.x = MaxWidth;
		mMaxSize.y = MaxHeight;

		if ( ForcePowOfTwo && !Math::isPow2( mMaxSize.x ) )
			mMaxSize.x = Math::nextPowOfTwo( mMaxSize.x );

		if ( ForcePowOfTwo && !Math::isPow2( mMaxSize.y ) )
			mMaxSize.y = Math::nextPowOfTwo( mMaxSize.y );

		mForcePowOfTwo = ForcePowOfTwo;
		mAllowFlipping = AllowFlipping;
		mPixelBorder = PixelBorder;
		mPixelDensity = PixelDensity;
		mTextureFilter = textureFilter;
		mScalableSVG = scalableSVG;
	}
}

void TexturePacker::newFree( Int32 x, Int32 y, Int32 width, Int32 height ) {
	TexturePackerNode* node = eeNew( TexturePackerNode, ( x, y, width, height ) );
	node->setNext( mFreeList );
	mFreeList = node;
}

bool TexturePacker::mergeNodes() {
	TexturePackerNode* f = mFreeList;

	while ( f ) {
		TexturePackerNode* prev = NULL;
		TexturePackerNode* c = mFreeList;

		while ( c ) {
			if ( f != c ) {
				if ( f->merge( *c ) ) {
					eeASSERT( prev );

					if ( NULL != prev ) {
						prev->setNext( c->getNext() );
					}

					eeSAFE_DELETE( c );

					return true;
				}
			}

			prev = c;
			c = c->getNext();
		}

		f = f->getNext();
	}

	return false;
}

void TexturePacker::validate() {
#ifdef EE_DEBUG
	TexturePackerNode* f = mFreeList;
	while ( f ) {
		TexturePackerNode* c = mFreeList;

		while ( c ) {
			if ( f != c )
				f->validate( c );

			c = c->getNext();
		}

		f = f->getNext();
	}
#endif
}

TexturePackerTex* TexturePacker::getLonguestEdge() {
	TexturePackerTex* t = NULL;
	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		if ( !( *it )->placed() ) {
			t = ( *it );
			break;
		}
	}

	return t;
}

TexturePackerTex* TexturePacker::getShortestEdge() {
	TexturePackerTex* t = NULL;
	std::list<TexturePackerTex*>::reverse_iterator it;

	for ( it = mTextures.rbegin(); it != mTextures.rend(); ++it ) {
		if ( !( *it )->placed() ) {
			t = ( *it );
			break;
		}
	}

	return t;
}

void TexturePacker::addBorderToTextures( const Int32& BorderSize ) {
	TexturePackerTex* t;

	if ( 0 != BorderSize ) {
		std::list<TexturePackerTex*>::iterator it;

		for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
			t = ( *it );

			t->width( t->width() + BorderSize );
			t->height( t->height() + BorderSize );
		}
	}
}

TexturePackerNode* TexturePacker::getBestFit( TexturePackerTex* t, TexturePackerNode** prevBestFit,
											  Int32* EdgeCount ) {
	Int32 leastY = 0x7FFFFFFF;
	Int32 leastX = 0x7FFFFFFF;
	TexturePackerNode* previousBestFit = NULL;
	TexturePackerNode* bestFit = NULL;
	TexturePackerNode* previous = NULL;
	TexturePackerNode* search = mFreeList;
	Int32 edgeCount = 0;

	// Walk the singly linked list of free nodes
	// see if it will fit into any currently free space
	while ( search ) {
		Int32 ec;

		// see if the texture will fit into this slot, and if so how many edges does it share.
		if ( search->fits( t->width(), t->height(), ec, mAllowFlipping ) ) {

			if ( ec == 2 ) {
				previousBestFit = previous; // record the pointer previous to this one (used to
											// patch the linked list)
				bestFit = search;			// record the best fit.
				edgeCount = ec;

				break;
			}

			if ( search->y() < leastY ) {
				leastY = search->y();
				leastX = search->x();
				previousBestFit = previous;
				bestFit = search;
				edgeCount = ec;
			} else if ( search->y() == leastY && search->x() < leastX ) {
				leastX = search->x();
				previousBestFit = previous;
				bestFit = search;
				edgeCount = ec;
			}
		}

		previous = search;
		search = search->getNext();
	}

	*EdgeCount = edgeCount;
	*prevBestFit = previousBestFit;

	return bestFit;
}

void TexturePacker::insertTexture( TexturePackerTex* t, TexturePackerNode* bestFit, Int32 edgeCount,
								   TexturePackerNode* previousBestFit ) {
	if ( NULL != bestFit ) {
		validate();

		switch ( edgeCount ) {
			case 0: {
				bool flipped = false;
				int w = t->width();
				int h = t->height();

				if ( mAllowFlipping ) {
					if ( t->longestEdge() <= bestFit->width() ) {
						if ( h > w ) {
							w = t->height();
							h = t->width();
							flipped = true;
						}
					} else {
						eeASSERT( t->longestEdge() <= bestFit->height() );

						if ( h < w ) {
							w = t->height();
							h = t->width();
							flipped = true;
						}
					}
				}

				t->place( bestFit->x(), bestFit->y(), flipped ); // place it.

				newFree( bestFit->x(), bestFit->y() + h, bestFit->width(), bestFit->height() - h );

				bestFit->x( bestFit->x() + w );
				bestFit->width( bestFit->width() - w );
				bestFit->height( h );

				validate();
			} break;
			case 1: {
				if ( t->width() == bestFit->width() ) {
					t->place( bestFit->x(), bestFit->y(), false );

					bestFit->y( bestFit->y() + t->height() );
					bestFit->height( bestFit->height() - t->height() );

					validate();
				} else if ( t->height() == bestFit->height() ) {
					t->place( bestFit->x(), bestFit->y(), false );

					bestFit->x( bestFit->x() + t->width() );
					bestFit->width( bestFit->width() - t->width() );

					validate();
				} else if ( mAllowFlipping && t->width() == bestFit->height() ) {
					t->place( bestFit->x(), bestFit->y(), true );

					bestFit->x( bestFit->x() + t->height() );
					bestFit->width( bestFit->width() - t->height() );

					validate();
				} else if ( mAllowFlipping && t->height() == bestFit->width() ) {
					t->place( bestFit->x(), bestFit->y(), true );

					bestFit->y( bestFit->y() + t->width() );
					bestFit->height( bestFit->height() - t->width() );

					validate();
				}
			} break;
			case 2: {
				bool flipped = t->width() != bestFit->width() || t->height() != bestFit->height();

				t->place( bestFit->x(), bestFit->y(), flipped );

				if ( previousBestFit )
					previousBestFit->setNext( bestFit->getNext() );
				else
					mFreeList = bestFit->getNext();

				eeSAFE_DELETE( bestFit );

				validate();
			} break;
		}

		while ( mergeNodes() )
			; // keep merging nodes as much as we can...
	}
}

void TexturePacker::createChild() {
	mChild = eeNew( TexturePacker, ( mWidth, mHeight, mPixelDensity, mForcePowOfTwo, mScalableSVG,
									 mPixelBorder, mTextureFilter, mAllowFlipping ) );

	std::list<TexturePackerTex*>::iterator it;
	std::list<std::list<TexturePackerTex*>::iterator> remove;

	TexturePackerTex* t = NULL;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		t = ( *it );

		if ( !t->placed() ) {
			mChild->addTexture( t->name() );
			mChild->mParent = this;

			t->disabled( true );

			remove.push_back( it );

			mCount--;
		}
	}

	// Removes the non-placed textures from the pack
	std::list<std::list<TexturePackerTex*>::iterator>::iterator itit;

	for ( itit = remove.begin(); itit != remove.end(); ++itit ) {
		mTextures.erase( *itit );
	}

	mChild->packTextures();
}

bool TexturePacker::addTexturesPath( std::string TexturesPath ) {
	if ( FileSystem::isDirectory( TexturesPath ) ) {
		FileSystem::dirPathAddSlashAtEnd( TexturesPath );

		std::vector<std::string> files = FileSystem::filesGetInPath( TexturesPath );
		std::sort( files.begin(), files.end() );

		for ( Uint32 i = 0; i < files.size(); i++ ) {
			std::string path( TexturesPath + files[i] );
			if ( !FileSystem::isDirectory( path ) && Image::isImageExtension( path ) )
				addTexture( path );
		}

		return true;
	}

	return false;
}

bool TexturePacker::addPackerTex( TexturePackerTex* TPack ) {
	if ( TPack->loadedInfo() ) {
		// Only add the texture if can fit inside the atlas, otherwise it will ignore it
		if ( ( TPack->width() + mPixelBorder <= mMaxSize.getWidth() &&
			   TPack->height() + mPixelBorder <= mMaxSize.getHeight() ) ||
			 ( mAllowFlipping && ( TPack->width() + mPixelBorder <= mMaxSize.getHeight() &&
								   TPack->height() + mPixelBorder <= mMaxSize.getWidth() ) ) ) {
			mTotalArea += TPack->area();

			// Insert ordered
			std::list<TexturePackerTex*>::iterator it;

			bool Added = false;

			for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
				if ( ( *it )->area() < TPack->area() ) {
					mTextures.insert( it, TPack );
					Added = true;
					break;
				}
			}

			if ( !Added ) {
				mTextures.push_back( TPack );

				return true;
			}
		}
	}

	return false;
}

bool TexturePacker::addImage( Image* Img, const std::string& Name ) {
	TexturePackerTex* TPack = eeNew( TexturePackerTex, ( Img, Name ) );

	return addPackerTex( TPack );
}

bool TexturePacker::addTexture( const std::string& TexturePath ) {
	if ( FileSystem::fileExists( TexturePath ) ) {
		Image::FormatConfiguration imageFormatConfiguration;

		imageFormatConfiguration.svgScale( mScalableSVG ? PixelDensity::toFloat( mPixelDensity )
														: 1.f );

		TexturePackerTex* TPack =
			eeNew( TexturePackerTex, ( TexturePath, imageFormatConfiguration ) );

		return addPackerTex( TPack );
	}

	return false;
}

Int32 TexturePacker::packTextures() { // pack the textures, the return code is the amount of
									  // wasted/unused area.
	TexturePackerTex* t = NULL;

	addBorderToTextures( (Int32)mPixelBorder );

	newFree( 0, 0, mWidth, mHeight );

	mCount = (Int32)mTextures.size();

	// We must place each texture
	std::list<TexturePackerTex*>::iterator it;
	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		// For the texture with the longest edge we place it according to this criteria.
		//   (1) If it is a perfect match, we always accept it as it causes the least amount of
		//   fragmentation. (2) A match of one edge with the minimum area left over after the split.
		//   (3) No edges match, so look for the node which leaves the least amount of area left
		//   over after the split.

		if ( PackBig == mStrategy ) {
			t = getLonguestEdge();
		} else if ( PackTiny == mStrategy ) {
			t = getShortestEdge();
		}

		TexturePackerNode* previousBestFit = NULL;
		Int32 edgeCount = 0;
		TexturePackerNode* bestFit = getBestFit( t, &previousBestFit, &edgeCount );

		if ( NULL == bestFit ) {
			if ( PackBig == mStrategy ) {
				eePRINTL( "Chaging Strategy to Tiny. %s faults.", t->name().c_str() );
				reset();
				addBorderToTextures( -( (Int32)mPixelBorder ) );
				mStrategy = PackTiny;
				return packTextures();
			} else if ( PackTiny == mStrategy ) {
				mStrategy = PackFail;
				eePRINTL( "Strategy fail, must expand image or create a new one. %s faults.",
						  t->name().c_str() );
			}
		} else {
			insertTexture( t, bestFit, edgeCount, previousBestFit );
			mCount--;
		}

		if ( PackFail == mStrategy ) {
			if ( mWidth < mMaxSize.getWidth() || mHeight < mMaxSize.getHeight() ) {
				reset();
				addBorderToTextures( -( (Int32)mPixelBorder ) );

				if ( mWidth <= mHeight ) {
					mWidth *= 2;

					if ( mWidth > mMaxSize.getWidth() )
						mWidth = mMaxSize.getWidth();
				} else {
					mHeight *= 2;

					if ( mHeight > mMaxSize.getHeight() )
						mHeight = mMaxSize.getHeight();
				}

				return packTextures();
			} else {
				eePRINTL( "Creating a new image as a child." );
				createChild();
			}

			break;
		}
	}

	if ( mCount > 0 ) {
		eePRINTL( "Creating a new image as a child. Some textures couldn't get it: %d", mCount );
		createChild();
	}

	addBorderToTextures( -( (Int32)mPixelBorder ) );

	mPacked = true;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		if ( !( *it )->placed() )
			mTotalArea -= ( *it )->area();
	}

	eePRINTL( "Total Area Used: %d. This represents the %4.3f percent", mTotalArea,
			  ( (double)mTotalArea / (double)( mWidth * mHeight ) ) * 100.0 );

	return ( mWidth * mHeight ) - mTotalArea;
}

void TexturePacker::save( const std::string& Filepath, const Image::SaveType& Format,
						  const bool& SaveExtensions ) {
	if ( !mPacked )
		packTextures();

	if ( !mTextures.size() )
		return;

	mFilepath = Filepath;
	mSaveExtensions = SaveExtensions;

	Image Img( (Uint32)mWidth, (Uint32)mHeight, getAtlasNumChannels() );

	Img.fillWithColor( Color( 0, 0, 0, 0 ) );

	TexturePackerTex* t = NULL;
	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		t = ( *it );

		if ( t->placed() ) {
			Uint8* data;

			if ( NULL == t->getImage() ) {
				Image imageLoaded( t->name() );

				if ( NULL != imageLoaded.getPixelsPtr() &&
					 t->width() == (int)imageLoaded.getWidth() &&
					 t->height() == (int)imageLoaded.getHeight() ) {
					if ( t->flipped() )
						imageLoaded.flip();

					Img.copyImage( &imageLoaded, t->x(), t->y() );

					mPlacedCount++;
				}
			} else {
				data = t->getImage()->getPixels();

				if ( NULL != data ) {
					if ( t->flipped() )
						t->getImage()->flip();

					Img.copyImage( t->getImage(), t->x(), t->y() );

					mPlacedCount++;
				}
			}
		}
	}

	mFormat = Format;

	Img.saveToFile( Filepath, Format );

	childSave( Format );

	saveTextureRegions();
}

Int32 TexturePacker::getChildCount() {
	TexturePacker* Child = mChild;
	Int32 ChildCount = 0;

	while ( NULL != Child ) {
		ChildCount++;
		Child = Child->getChild();
	}

	return ChildCount;
}

void TexturePacker::saveTextureRegions() {
	if ( NULL != mParent )
		return;

	sTextureAtlasHdr TexGrHdr;

	TexGrHdr.Magic = EE_TEXTURE_ATLAS_MAGIC;
	TexGrHdr.Version = 1000;
	TexGrHdr.Date = static_cast<Uint64>( Sys::getSystemTime() );
	TexGrHdr.TextureCount = 1 + getChildCount();
	TexGrHdr.Format = mFormat;
	TexGrHdr.Width = mWidth;
	TexGrHdr.Height = mHeight;
	TexGrHdr.PixelBorder = mPixelBorder;
	TexGrHdr.Flags = 0;
	TexGrHdr.TextureFilter = mTextureFilter;

	int reservedSize = eeARRAY_SIZE( TexGrHdr.Reserved );
	memset( TexGrHdr.Reserved, 0, reservedSize );

	if ( mAllowFlipping )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_ALLOW_FLIPPING;

	if ( !mSaveExtensions )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_REMOVE_EXTENSION;

	if ( mForcePowOfTwo )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_POW_OF_TWO;

	if ( mScalableSVG )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_SCALABLE_SVG;

	std::vector<sTextureHdr> TexHdr( TexGrHdr.TextureCount );

	TexHdr[0] = createTextureHdr( this );

	Int32 HdrPos = 1;
	TexturePacker* Child = mChild;

	while ( NULL != Child ) {
		TexHdr[HdrPos] = createTextureHdr( Child );
		Child = Child->getChild();
		HdrPos++;
	}

	std::vector<sTextureRegionHdr> tTextureRegionsHdr;

	std::string path = FileSystem::fileRemoveExtension( mFilepath ) + EE_TEXTURE_ATLAS_EXTENSION;
	IOStreamFile fs( path, "wb" );

	if ( fs.isOpen() ) {
		fs.write( reinterpret_cast<const char*>( &TexGrHdr ), sizeof( sTextureAtlasHdr ) );

		fs.write( reinterpret_cast<const char*>( &TexHdr[0] ), sizeof( sTextureHdr ) );

		createTextureRegionsHdr( this, tTextureRegionsHdr );

		if ( tTextureRegionsHdr.size() )
			fs.write( reinterpret_cast<const char*>( &tTextureRegionsHdr[0] ),
					  sizeof( sTextureRegionHdr ) * (std::streamsize)tTextureRegionsHdr.size() );

		Int32 HdrPos = 1;
		TexturePacker* Child = mChild;

		while ( NULL != Child ) {
			fs.write( reinterpret_cast<const char*>( &TexHdr[HdrPos] ), sizeof( sTextureHdr ) );

			createTextureRegionsHdr( Child, tTextureRegionsHdr );

			if ( tTextureRegionsHdr.size() )
				fs.write( reinterpret_cast<const char*>( &tTextureRegionsHdr[0] ),
						  sizeof( sTextureRegionHdr ) *
							  (std::streamsize)tTextureRegionsHdr.size() );

			Child = Child->getChild();

			HdrPos++;
		}
	}
}

void TexturePacker::createTextureRegionsHdr( TexturePacker* Packer,
											 std::vector<sTextureRegionHdr>& TextureRegions ) {
	TextureRegions.clear();

	sTextureRegionHdr tTextureRegionHdr;
	Uint32 c = 0;

	std::list<TexturePackerTex*> tTextures = *( Packer->getTexturePackPtr() );
	std::list<TexturePackerTex*>::iterator it;
	TexturePackerTex* tTex;

	TextureRegions.resize( tTextures.size() );

	for ( it = tTextures.begin(); it != tTextures.end(); ++it ) {
		tTex = ( *it );

		if ( tTex->placed() ) {
			std::string name = FileSystem::fileNameFromPath( tTex->name() );

			memset( tTextureRegionHdr.Name, 0, HDR_NAME_SIZE );

			String::strCopy( tTextureRegionHdr.Name, name.c_str(), HDR_NAME_SIZE );

			if ( !mSaveExtensions )
				name = FileSystem::fileRemoveExtension( name );

			tTextureRegionHdr.ResourceID = String::hash( name );
			tTextureRegionHdr.Width = tTex->width();
			tTextureRegionHdr.Height = tTex->height();
			tTextureRegionHdr.Channels = tTex->channels();
			tTextureRegionHdr.DestWidth = tTex->width();
			tTextureRegionHdr.DestHeight = tTex->height();
			tTextureRegionHdr.OffsetX = 0;
			tTextureRegionHdr.OffsetY = 0;
			tTextureRegionHdr.X = tTex->x();
			tTextureRegionHdr.Y = tTex->y();
			tTextureRegionHdr.Date = FileSystem::fileGetModificationDate( tTex->name() );
			tTextureRegionHdr.Flags = 0;
			tTextureRegionHdr.PixelDensity = (Uint32)mPixelDensity;

			if ( tTex->flipped() )
				tTextureRegionHdr.Flags |= HDR_TEXTUREREGION_FLAG_FLIPED;

			TextureRegions[c] = tTextureRegionHdr;

			c++;
		}
	}
}

sTextureHdr TexturePacker::createTextureHdr( TexturePacker* Packer ) {
	sTextureHdr TexHdr;

	std::string name( FileSystem::fileNameFromPath( Packer->getFilepath() ) );

	memset( TexHdr.Name, 0, HDR_NAME_SIZE );

	String::strCopy( TexHdr.Name, name.c_str(), HDR_NAME_SIZE );

	TexHdr.ResourceID = String::hash( name );
	TexHdr.Size = FileSystem::fileSize( Packer->getFilepath() );
	TexHdr.TextureRegionCount = Packer->getPlacedCount();

	return TexHdr;
}

void TexturePacker::childSave( const Image::SaveType& Format ) {
	if ( NULL != mChild ) {
		TexturePacker* Parent = mChild->getParent();
		TexturePacker* LastParent = NULL;
		Int32 ParentCount = 0;

		// Find the grand parent
		while ( NULL != Parent ) {
			ParentCount++;
			LastParent = Parent;
			Parent = Parent->getParent();
		}

		if ( NULL != LastParent ) {
			std::string fFpath = FileSystem::fileRemoveExtension( LastParent->getFilepath() );
			std::string fExt = FileSystem::fileExtension( LastParent->getFilepath() );
			std::string fName = fFpath + "_ch" + String::toStr( ParentCount ) + "." + fExt;

			mChild->save( fName, Format, mSaveExtensions );
		}
	}
}

TexturePacker* TexturePacker::getChild() const {
	return mChild;
}

TexturePacker* TexturePacker::getParent() const {
	return mParent;
}

std::list<TexturePackerTex*>* TexturePacker::getTexturePackPtr() {
	return &mTextures;
}

const std::string& TexturePacker::getFilepath() const {
	return mFilepath;
}

const Int32& TexturePacker::getWidth() const {
	return mWidth;
}

const Int32& TexturePacker::getHeight() const {
	return mHeight;
}

const Int32& TexturePacker::getPlacedCount() const {
	return mPlacedCount;
}

}} // namespace EE::Graphics
