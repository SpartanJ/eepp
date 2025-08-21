#include <algorithm>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/graphics/texturepackernode.hpp>
#include <eepp/graphics/texturepackertex.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/sys.hpp>

namespace EE { namespace Graphics {

TexturePacker* TexturePacker::New() {
	return eeNew( TexturePacker, () );
}

TexturePacker* TexturePacker::New( const Uint32& maxWidth, const Uint32& maxHeight,
								   const Float& pixelDensity, const bool& forcePowOfTwo,
								   const bool& scalableSVG, const Uint32& pixelBorder,
								   const Texture::Filter& textureFilter, const bool& allowChildren,
								   const bool& allowFlipping ) {
	return eeNew( TexturePacker, ( maxWidth, maxHeight, pixelDensity, forcePowOfTwo, scalableSVG,
								   pixelBorder, textureFilter, allowChildren, allowFlipping ) );
}

TexturePacker::TexturePacker( const Uint32& maxWidth, const Uint32& maxHeight,
							  const Float& pixelDensity, const bool& forcePowOfTwo,
							  const bool& scalableSVG, const Uint32& pixelBorder,
							  const Texture::Filter& textureFilter, const bool& allowChildren,
							  const bool& allowFlipping ) :
	mTotalArea( 0 ),
	mFreeList( NULL ),
	mWidth( 128 ),
	mHeight( 128 ),
	mPacked( false ),
	mAllowFlipping( allowFlipping ),
	mAllowChildren( allowChildren ),
	mChild( NULL ),
	mStrategy( PackBig ),
	mCount( 0 ),
	mParent( NULL ),
	mPlacedCount( 0 ),
	mForcePowOfTwo( true ),
	mPixelBorder( 0 ),
	mPixelDensity( eefloor( pixelDensity * 100 ) ),
	mTextureFilter( textureFilter ),
	mKeepExtensions( false ),
	mScalableSVG( scalableSVG ),
	mFormat( Image::SaveType::PNG ) {
	setOptions( maxWidth, maxHeight, pixelDensity, forcePowOfTwo, scalableSVG, pixelBorder,
				textureFilter, allowChildren, allowFlipping );
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
	mPixelDensity( 1 ),
	mTextureFilter( Texture::Filter::Linear ),
	mKeepExtensions( false ),
	mScalableSVG( false ),
	mFormat( Image::SaveType::PNG ) {}

TexturePacker::~TexturePacker() {
	close();
}

void TexturePacker::close() {
	reset();

	std::vector<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		eeSAFE_DELETE( ( *it ) );
	}

	mTextures.clear();
}

void TexturePacker::reset() {
	mStrategy = PackBig;
	mCount = mTextures.size();

	TexturePackerTex* t = NULL;
	std::vector<TexturePackerTex*>::iterator it;

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
	std::vector<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		t = ( *it );

		if ( t->placed() ) {
			maxChannels = eemax( maxChannels, (Uint32)t->channels() );
		}
	}

	return maxChannels;
}

void TexturePacker::setOptions( const Uint32& maxWidth, const Uint32& maxHeight,
								const Float& pixelDensity, const bool& forcePowOfTwo,
								const bool& scalableSVG, const Uint32& pixelBorder,
								const Texture::Filter& textureFilter, const bool& allowChildren,
								const bool& allowFlipping ) {
	if ( !mTextures.size() ) { // only can change the dimensions before adding any texture
		mMaxSize.x = maxWidth;
		mMaxSize.y = maxHeight;

		if ( forcePowOfTwo && !Math::isPow2( mMaxSize.x ) )
			mMaxSize.x = Math::nextPowOfTwo( mMaxSize.x );

		if ( forcePowOfTwo && !Math::isPow2( mMaxSize.y ) )
			mMaxSize.y = Math::nextPowOfTwo( mMaxSize.y );

		mForcePowOfTwo = forcePowOfTwo;
		mAllowFlipping = allowFlipping;
		mAllowChildren = allowChildren;
		mPixelBorder = pixelBorder;
		mPixelDensity = eefloor( pixelDensity * 100 );
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

TexturePackerTex* TexturePacker::getLongestEdge() {
	TexturePackerTex* t = NULL;
	std::vector<TexturePackerTex*>::iterator it;

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
	std::vector<TexturePackerTex*>::reverse_iterator it;

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
		std::vector<TexturePackerTex*>::iterator it;

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
	mChild = TexturePacker::New( mWidth, mHeight, mPixelDensity / 100.f, mForcePowOfTwo,
								 mScalableSVG, mPixelBorder, mTextureFilter, mAllowFlipping );

	std::vector<TexturePackerTex*>::iterator it;
	std::vector<std::vector<TexturePackerTex*>::iterator> remove;

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
	std::vector<std::vector<TexturePackerTex*>::iterator>::iterator itit;

	for ( itit = remove.begin(); itit != remove.end(); ++itit ) {
		mTextures.erase( *itit );
	}

	mChild->packTextures();
}

bool TexturePacker::addTexturesPath( std::string TexturesPath ) {
	if ( FileSystem::isDirectory( TexturesPath ) ) {
		FileSystem::dirAddSlashAtEnd( TexturesPath );

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
			std::vector<TexturePackerTex*>::iterator it;

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

		imageFormatConfiguration.svgScale( mScalableSVG ? mPixelDensity / 100.f : 1.f );

		TexturePackerTex* TPack =
			eeNew( TexturePackerTex, ( TexturePath, imageFormatConfiguration ) );

		return addPackerTex( TPack );
	}

	return false;
}

Int32 TexturePacker::packTextures() {
	TexturePackerTex* t = NULL;

	addBorderToTextures( (Int32)mPixelBorder );

	newFree( 0, 0, mWidth, mHeight );

	mCount = (Int32)mTextures.size();

	// We must place each texture
	std::vector<TexturePackerTex*>::iterator it;
	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		// For the texture with the longest edge we place it according to this criteria.
		//   (1) If it is a perfect match, we always accept it as it causes the least amount of
		//   fragmentation. (2) A match of one edge with the minimum area left over after the split.
		//   (3) No edges match, so look for the node which leaves the least amount of area left
		//   over after the split.

		if ( PackBig == mStrategy ) {
			t = getLongestEdge();
		} else if ( PackTiny == mStrategy ) {
			t = getShortestEdge();
		}

		TexturePackerNode* previousBestFit = NULL;
		Int32 edgeCount = 0;
		TexturePackerNode* bestFit = getBestFit( t, &previousBestFit, &edgeCount );

		if ( NULL == bestFit ) {
			if ( PackBig == mStrategy ) {
				Log::debug( "TexturePacker: Changing Strategy to Tiny. %s faults.",
							t->name().c_str() );
				reset();
				addBorderToTextures( -( (Int32)mPixelBorder ) );
				mStrategy = PackTiny;
				return packTextures();
			} else if ( PackTiny == mStrategy ) {
				mStrategy = PackFail;
				Log::warning( "TexturePacker: Strategy fail, must expand image or create a new "
							  "one. %s faults.",
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
				if ( !mAllowChildren ) {
					return 0;
				}
			}

			break;
		}
	}

	if ( mCount > 0 ) {
		if ( mAllowChildren ) {
			Log::debug( "Creating a new image as a child. Some textures couldn't get it: %d",
						mCount );
			createChild();
		} else {
			return 0;
		}
	}

	addBorderToTextures( -( (Int32)mPixelBorder ) );

	mPacked = true;

	for ( it = mTextures.begin(); it != mTextures.end(); ++it ) {
		if ( !( *it )->placed() )
			mTotalArea -= ( *it )->area();
	}

	Log::debug( "Total Area Used: %d. This represents the %4.3f percent", mTotalArea,
				( (double)mTotalArea / (double)( mWidth * mHeight ) ) * 100.0 );

	return mTotalArea;
}

void TexturePacker::save( const std::string& Filepath, const Image::SaveType& Format,
						  const bool& KeepExtensions ) {
	if ( !mPacked )
		packTextures();

	if ( !mTextures.size() )
		return;

	mFilepath = Filepath;
	mKeepExtensions = KeepExtensions;

	Image Img( (Uint32)mWidth, (Uint32)mHeight, getAtlasNumChannels() );

	Img.fillWithColor( Color( 0, 0, 0, 0 ) );

	TexturePackerTex* t = NULL;
	std::vector<TexturePackerTex*>::iterator it;

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
	sTextureAtlasHdr TexGrHdr;

	TexGrHdr.Magic = EE_TEXTURE_ATLAS_MAGIC;
	TexGrHdr.Version = HDR_TEXTURE_ATLAS_VERSION;
	TexGrHdr.Date = static_cast<Uint64>( Sys::getSystemTime() );
	TexGrHdr.TextureCount = 1;
	TexGrHdr.Format = static_cast<int>( mFormat );
	TexGrHdr.Width = mWidth;
	TexGrHdr.Height = mHeight;
	TexGrHdr.PixelBorder = mPixelBorder;
	TexGrHdr.Flags = 0;
	TexGrHdr.TextureFilter = (Uint32)mTextureFilter;

	int reservedSize = eeARRAY_SIZE( TexGrHdr.Reserved );
	memset( TexGrHdr.Reserved, 0, reservedSize );

	if ( mAllowFlipping )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_ALLOW_FLIPPING;

	if ( !mKeepExtensions )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_REMOVE_EXTENSION;

	if ( mForcePowOfTwo )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_POW_OF_TWO;

	if ( mScalableSVG )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_SCALABLE_SVG;

	std::vector<sTextureHdr> TexHdr( TexGrHdr.TextureCount );

	TexHdr[0] = createTextureHdr( this );

	std::vector<sTextureRegionHdr> tTextureRegionsHdr;

	std::string path = FileSystem::fileRemoveExtension( mFilepath ) + EE_TEXTURE_ATLAS_EXTENSION;
	IOStreamFile fs( path, "wb" );

	if ( fs.isOpen() ) {
		fs.write( reinterpret_cast<const char*>( &TexGrHdr ), sizeof( sTextureAtlasHdr ) );

		fs.write( reinterpret_cast<const char*>( &TexHdr[0] ), sizeof( sTextureHdr ) );

		createTextureRegionsHdr( this, tTextureRegionsHdr );

		if ( tTextureRegionsHdr.size() )
			fs.write( reinterpret_cast<const char*>( &tTextureRegionsHdr[0] ),
					  sizeof( sTextureRegionHdr ) * tTextureRegionsHdr.size() );
	}
}

void TexturePacker::createTextureRegionsHdr( TexturePacker* Packer,
											 std::vector<sTextureRegionHdr>& TextureRegions ) {
	TextureRegions.clear();

	sTextureRegionHdr tTextureRegionHdr;
	Uint32 c = 0;

	std::vector<TexturePackerTex*> tTextures = *( Packer->getTexturePackPtr() );
	std::vector<TexturePackerTex*>::iterator it;
	TexturePackerTex* tTex;

	TextureRegions.resize( tTextures.size() );

	for ( it = tTextures.begin(); it != tTextures.end(); ++it ) {
		tTex = ( *it );

		if ( tTex->placed() ) {
			std::string name = FileSystem::fileNameFromPath( tTex->name() );

			if ( name.size() > HDR_NAME_SIZE )
				name.resize( HDR_NAME_SIZE );

			memset( tTextureRegionHdr.Name, 0, HDR_NAME_SIZE );

			String::strCopy( tTextureRegionHdr.Name, name.c_str(), HDR_NAME_SIZE );

			if ( !mKeepExtensions )
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
			tTextureRegionHdr.PixelDensity = mPixelDensity;
			MD5::Result md5Result = MD5::fromFile( tTex->name() );
			memcpy( tTextureRegionHdr.Hash, &md5Result.digest[0], HDR_HASH_SIZE );

			if ( tTex->flipped() )
				tTextureRegionHdr.Flags |= HDR_TEXTUREREGION_FLAG_FLIPPED;

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
			std::string fName = fFpath + "-ch" + String::toString( ParentCount ) + "." + fExt;

			mChild->save( fName, Format, mKeepExtensions );
		}
	}
}

TexturePacker* TexturePacker::getChild() const {
	return mChild;
}

TexturePacker* TexturePacker::getParent() const {
	return mParent;
}

std::vector<TexturePackerTex*>* TexturePacker::getTexturePackPtr() {
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
