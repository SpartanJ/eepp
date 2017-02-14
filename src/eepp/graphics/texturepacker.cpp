#include <eepp/graphics/texturepacker.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/graphics/texturepackernode.hpp>
#include <eepp/graphics/texturepackertex.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <algorithm>

namespace EE { namespace Graphics {

TexturePacker::TexturePacker( const Uint32& MaxWidth, const Uint32& MaxHeight, const bool& ForcePowOfTwo, const Uint32& PixelBorder, const bool& AllowFlipping ) :
	mLongestEdge(0),
	mTotalArea(0),
	mFreeList(NULL),
	mWidth(0),
	mHeight(0),
	mPacked(false),
	mAllowFlipping(false),
	mChild(NULL),
	mStrategy(PackBig),
	mCount(0),
	mParent(NULL),
	mPlacedCount(0),
	mForcePowOfTwo(true),
	mPixelBorder(0)
{
	SetOptions( MaxWidth, MaxHeight, ForcePowOfTwo, PixelBorder, AllowFlipping );
}

TexturePacker::TexturePacker() :
	mLongestEdge(0),
	mTotalArea(0),
	mFreeList(NULL),
	mWidth(1024),
	mHeight(1024),
	mPacked(false),
	mAllowFlipping(false),
	mChild(NULL),
	mStrategy(PackBig),
	mCount(0),
	mParent(NULL),
	mPlacedCount(0),
	mForcePowOfTwo(true),
	mPixelBorder(0)
{
}

TexturePacker::~TexturePacker()
{
	Close();
}

void TexturePacker::Close() {
	mLongestEdge 	= 0;
	mTotalArea 		= 0;

	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		eeSAFE_DELETE( (*it) );
	}

	mTextures.clear();

	if ( NULL != mFreeList ) {
		TexturePackerNode * next = mFreeList;
		TexturePackerNode * kill = NULL;

		while ( NULL != next ) {
			kill = next;

			next = next->GetNext();

			eeSAFE_DELETE( kill );
		}
	}

	eeSAFE_DELETE( mChild );
}

void TexturePacker::SetOptions( const Uint32& MaxWidth, const Uint32& MaxHeight, const bool& ForcePowOfTwo, const Uint32& PixelBorder, const bool& AllowFlipping ) {
	if ( !mTextures.size() ) { // only can change the dimensions before adding any texture
		mWidth 	= MaxWidth;
		mHeight = MaxHeight;

		if ( ForcePowOfTwo && !Math::IsPow2( mWidth ) )
			mWidth = Math::NextPowOfTwo( mWidth );

		if ( ForcePowOfTwo && !Math::IsPow2( mHeight ) )
			mHeight = Math::NextPowOfTwo( mHeight );

		mForcePowOfTwo 	= ForcePowOfTwo;
		mAllowFlipping 	= AllowFlipping;
		mPixelBorder	= PixelBorder;
	}
}

void TexturePacker::NewFree( Int32 x, Int32 y, Int32 width, Int32 height ) {
	TexturePackerNode * node = eeNew( TexturePackerNode, ( x, y, width, height ) );
	node->SetNext( mFreeList );
	mFreeList = node;
}

bool TexturePacker::MergeNodes() {
	TexturePackerNode *f = mFreeList;

	while ( f ) {
		TexturePackerNode * prev 	= NULL;
		TexturePackerNode * c 		= mFreeList;

		while ( c ) {
			if ( f != c ) {
				if ( f->Merge( *c ) ) {
					eeASSERT( prev );

					if ( NULL != prev ) {
						prev->SetNext( c->GetNext() );
					}

					eeSAFE_DELETE( c );

					return true;
				}
			}

			prev 	= c;
			c 		= c->GetNext();
		}

		f = f->GetNext();
	}

	return false;
}

void TexturePacker::Validate() {
#ifdef EE_DEBUG
	TexturePackerNode * f = mFreeList;
	while ( f ) {
		TexturePackerNode * c = mFreeList;

		while ( c ) {
			if ( f != c )
				f->Validate(c);

			c = c->GetNext();
		}

		f = f->GetNext();
	}
#endif
}

TexturePackerTex * TexturePacker::GetLonguestEdge() {
	TexturePackerTex * t = NULL;
	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		if ( !(*it)->Placed() ) {
			t = (*it);
			break;
		}
	}

	return t;
}

TexturePackerTex * TexturePacker::GetShortestEdge() {
	TexturePackerTex * t = NULL;
	std::list<TexturePackerTex*>::reverse_iterator it;

	for ( it = mTextures.rbegin(); it != mTextures.rend(); it++ ) {
		if ( !(*it)->Placed() ) {
			t = (*it);
			break;
		}
	}

	return t;
}

void TexturePacker::AddBorderToTextures( const Int32& BorderSize ) {
	TexturePackerTex * t;

	if ( 0 != BorderSize ) {
		std::list<TexturePackerTex*>::iterator it;

		for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
			t = (*it);

			t->Width	( t->Width() 	+ BorderSize );
			t->Height	( t->Height() 	+ BorderSize );
		}

		mLongestEdge += BorderSize;
	}
}

TexturePackerNode *	TexturePacker::GetBestFit( TexturePackerTex * t, TexturePackerNode ** prevBestFit, Int32 * EdgeCount ) {
	Int32 leastY 							= 0x7FFFFFFF;
	Int32 leastX 							= 0x7FFFFFFF;
	TexturePackerNode * previousBestFit 	= NULL;
	TexturePackerNode * bestFit 			= NULL;
	TexturePackerNode * previous 			= NULL;
	TexturePackerNode * search 				= mFreeList;
	Int32 edgeCount 						= 0;

	// Walk the singly linked list of free nodes
	// see if it will fit into any currently free space
	while ( search ) {
		Int32 ec;

		// see if the texture will fit into this slot, and if so how many edges does it share.
		if ( search->Fits( t->Width(), t->Height(), ec, mAllowFlipping ) ) {

			if ( ec == 2 ) {
				previousBestFit 	= previous; // record the pointer previous to this one (used to patch the linked list)
				bestFit 			= search; 	// record the best fit.
				edgeCount 			= ec;

				break;
			}

			if ( search->Y() < leastY ) {
				leastY 				= search->Y();
				leastX 				= search->X();
				previousBestFit 	= previous;
				bestFit 			= search;
				edgeCount 			= ec;
			} else if ( search->Y() == leastY && search->X() < leastX ) {
				leastX 				= search->X();
				previousBestFit 	= previous;
				bestFit 			= search;
				edgeCount 			= ec;
			}
		}

		previous 	= search;
		search 		= search->GetNext();
	}

	*EdgeCount 		= edgeCount;
	*prevBestFit	= previousBestFit;

	return bestFit;
}

void TexturePacker::InsertTexture( TexturePackerTex * t, TexturePackerNode * bestFit, Int32 edgeCount, TexturePackerNode * previousBestFit ) {
	if ( NULL != bestFit ) {
		Validate();

		switch ( edgeCount ) {
			case 0:
			{
				bool flipped 	= false;
				int w 			= t->Width();
				int h 			= t->Height();

				if ( mAllowFlipping ) {
					if ( t->LongestEdge() <= bestFit->Width() ) {
						if ( h > w ) {
							w 		= t->Height();
							h 		= t->Width();
							flipped = true;
						}
					} else {
						eeASSERT( t->LongestEdge() <= bestFit->Height() );

						if ( h < w ) {
							w 		= t->Height();
							h 		= t->Width();
							flipped = true;
						}
					}
				}

				t->Place( bestFit->X(), bestFit->Y(), flipped ); // place it.

				NewFree( bestFit->X(), bestFit->Y() + h, bestFit->Width(), bestFit->Height() - h );

				bestFit->X		( bestFit->X() 		+ w );
				bestFit->Width	( bestFit->Width() 	- w );
				bestFit->Height	( h 					);

				Validate();
			}
			break;
			case 1:
			{
				if ( t->Width() == bestFit->Width() ) {
					t->Place( bestFit->X(), bestFit->Y(), false );

					bestFit->Y		( bestFit->Y() 		+ t->Height() );
					bestFit->Height	( bestFit->Height() - t->Height() );

					Validate();
				} else if ( t->Height() == bestFit->Height() ) {
					t->Place( bestFit->X(), bestFit->Y(), false );

					bestFit->X		( bestFit->X() 		+ t->Width() );
					bestFit->Width	( bestFit->Width() 	- t->Width() );

					Validate();
				} else if ( mAllowFlipping && t->Width() == bestFit->Height() ) {
					t->Place( bestFit->X(), bestFit->Y(), true );

					bestFit->X		( bestFit->X() 		+ t->Height() );
					bestFit->Width	( bestFit->Width() 	- t->Height() );

					Validate();
				} else if ( mAllowFlipping && t->Height() == bestFit->Width() ) {
					t->Place( bestFit->X(), bestFit->Y(), true );

					bestFit->Y		( bestFit->Y() 		+ t->Width() );
					bestFit->Height	( bestFit->Height() - t->Width() );

					Validate();
				}
			}
			break;
			case 2:
			{
				bool flipped = t->Width() != bestFit->Width() || t->Height() != bestFit->Height();

				t->Place( bestFit->X(), bestFit->Y(), flipped );

				if ( previousBestFit )
					previousBestFit->SetNext( bestFit->GetNext() );
				else
					mFreeList = bestFit->GetNext();

				eeSAFE_DELETE( bestFit );

				Validate();
			}
			break;
		}

		while ( MergeNodes() ); // keep merging nodes as much as we can...
	}
}

void TexturePacker::CreateChild() {
	mChild = eeNew( TexturePacker, ( mWidth, mHeight, mForcePowOfTwo, mPixelBorder, mAllowFlipping ) );

	std::list<TexturePackerTex*>::iterator it;
	std::list< std::list<TexturePackerTex*>::iterator > remove;

	TexturePackerTex * t = NULL;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		t = (*it);

		if ( !t->Placed() ) {
			mChild->AddTexture( t->Name() );
			mChild->mParent = this;

			t->Disabled( true );

			remove.push_back( it );

			mCount--;
		}
	}

	// Removes the non-placed textures from the pack
	std::list< std::list<TexturePackerTex*>::iterator >::iterator itit;

	for ( itit = remove.begin(); itit != remove.end(); itit++ ) {
		mTextures.erase( *itit );
	}

	mChild->PackTextures();
}

bool TexturePacker::AddTexturesPath( std::string TexturesPath ) {
	if ( FileSystem::isDirectory( TexturesPath ) ) {

		FileSystem::dirPathAddSlashAtEnd( TexturesPath );

		std::vector<std::string > files = FileSystem::filesGetInPath( TexturesPath );
		std::sort( files.begin(), files.end() );

		for ( Uint32 i = 0; i < files.size(); i++ ) {
			std::string path( TexturesPath + files[i] );
			if ( !FileSystem::isDirectory( path ) )
				AddTexture( path );
		}

		return true;
	}

	return false;
}

bool TexturePacker::AddPackerTex( TexturePackerTex * TPack ) {
	if ( TPack->LoadedInfo() ) {
		// Only add the texture if can fit inside the atlas, otherwise it will ignore it
		if ( ( TPack->Width() + mPixelBorder <= mWidth && TPack->Height() + mPixelBorder <= mHeight ) ||
			( mAllowFlipping && ( TPack->Width() + mPixelBorder <= mHeight && TPack->Height() + mPixelBorder <= mWidth ) )
		)
		{
			mTotalArea += TPack->Area();

			// Insert ordered
			std::list<TexturePackerTex*>::iterator it;

			bool Added = false;

			for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
				if ( (*it)->Area() < TPack->Area() ) {
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

bool TexturePacker::AddImage( Image * Img, const std::string& Name ) {
	TexturePackerTex * TPack = eeNew( TexturePackerTex, ( Img, Name ) );
	return AddPackerTex( TPack );
}

bool TexturePacker::AddTexture( const std::string& TexturePath ) {
	if ( FileSystem::fileExists( TexturePath ) ) {
		TexturePackerTex * TPack = eeNew( TexturePackerTex, ( TexturePath ) );
		return AddPackerTex( TPack );
	}

	return false;
}

Int32 TexturePacker::PackTextures() { // pack the textures, the return code is the amount of wasted/unused area.
	if ( mWidth <= 0 || mHeight <= 0 )
		return 0;

	TexturePackerTex * t 	= NULL;

	AddBorderToTextures( (Int32)mPixelBorder );

	NewFree( 0, 0, mWidth, mHeight );

	mCount = (Int32)mTextures.size();

	// We must place each texture
	std::list<TexturePackerTex*>::iterator it;
	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		// For the texture with the longest edge we place it according to this criteria.
		//   (1) If it is a perfect match, we always accept it as it causes the least amount of fragmentation.
		//   (2) A match of one edge with the minimum area left over after the split.
		//   (3) No edges match, so look for the node which leaves the least amount of area left over after the split.

		if ( PackBig == mStrategy )
			t 									= GetLonguestEdge();
		else if ( PackTiny == mStrategy )
			t 									= GetShortestEdge();

		TexturePackerNode * previousBestFit = NULL;
		Int32 edgeCount 					= 0;
		TexturePackerNode * bestFit 		= GetBestFit( t, &previousBestFit, &edgeCount );

		if ( NULL == bestFit ) {
			if ( PackBig == mStrategy ) {
				mStrategy = PackTiny;
				eePRINTL( "Chaging Strategy to Tiny. %s faults.", t->Name().c_str() );
			} else if ( PackTiny == mStrategy ) {
				mStrategy = PackFail;
				eePRINTL( "Strategy fail, must create a new image. %s faults.", t->Name().c_str() );
			}
		} else {
			InsertTexture( t, bestFit, edgeCount, previousBestFit );
			mCount--;
		}

		if ( PackFail == mStrategy ) {
			eePRINTL( "Creating a new image as a child." );
			CreateChild();
			break;
		}
	}

	if ( mCount > 0 ) {
		eePRINTL( "Creating a new image as a child. Some textures couldn't get it: %d", mCount );
		CreateChild();
	}

	AddBorderToTextures( -( (Int32)mPixelBorder ) );

	mPacked = true;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		if ( !(*it)->Placed() )
			mTotalArea -= (*it)->Area();
	}

	eePRINTL( "Total Area Used: %d. This represents the %4.3f percent", mTotalArea, ( (double)mTotalArea / (double)( mWidth * mHeight ) ) * 100.0 );

	return ( mWidth * mHeight ) - mTotalArea;
}

void TexturePacker::Save( const std::string& Filepath, const EE_SAVE_TYPE& Format, const bool& SaveExtensions ) {
	if ( !mPacked )
		PackTextures();

	if ( !mTextures.size() )
		return;

	mFilepath = Filepath;
	mSaveExtensions = SaveExtensions;

	Image Img( (Uint32)mWidth, (Uint32)mHeight, (Uint32)4 );

	Img.FillWithColor( ColorA(0,0,0,0) );

	TexturePackerTex * t = NULL;
	int w, h, c;
	std::list<TexturePackerTex*>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		t = (*it);

		if ( t->Placed() ) {
			Uint8 * data;

			if ( NULL == t->Image() ) {
				data = stbi_load( t->Name().c_str(), &w, &h, &c, 0 );

				if ( NULL != data && t->Width() == w && t->Height() == h ) {
					Image * ImgCopy = eeNew( Image, ( data, w, h, c ) );

					if ( t->Flipped() )
						ImgCopy->Flip();

					Img.CopyImage( ImgCopy, t->X(), t->Y() );

					ImgCopy->AvoidFreeImage( true );

					eeSAFE_DELETE( ImgCopy );

					if ( data )
						free( data );

					mPlacedCount++;
				}
			} else {
				data = t->Image()->GetPixels();

				if ( NULL != data ) {
					if ( t->Flipped() )
						t->Image()->Flip();

					Img.CopyImage( t->Image(), t->X(), t->Y() );

					mPlacedCount++;
				}
			}
		}
	}

	mFormat = Format;

	Img.SaveToFile( Filepath, Format );

	ChildSave( Format );

	SaveSubTextures();
}

Int32 TexturePacker::GetChildCount() {
	TexturePacker * Child 		= mChild;
	Int32 ChildCount 			= 0;

	while ( NULL != Child ) {
		ChildCount++;
		Child = Child->GetChild();
	}

	return ChildCount;
}

void TexturePacker::SaveSubTextures() {
	if ( NULL != mParent )
		return;

	sTextureAtlasHdr TexGrHdr;

	TexGrHdr.Magic 			= EE_TEXTURE_ATLAS_MAGIC;
	TexGrHdr.TextureCount 	= 1 + GetChildCount();
	TexGrHdr.Format			= mFormat;
	TexGrHdr.Width			= mWidth;
	TexGrHdr.Height			= mHeight;
	TexGrHdr.PixelBorder	= mPixelBorder;
	TexGrHdr.Flags			= 0;

	if ( mAllowFlipping )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_ALLOW_FLIPPING;

	if ( !mSaveExtensions )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_REMOVE_EXTENSION;

	if ( mForcePowOfTwo )
		TexGrHdr.Flags |= HDR_TEXTURE_ATLAS_POW_OF_TWO;

	std::vector<sTextureHdr> TexHdr( TexGrHdr.TextureCount );

	TexHdr[ 0 ] = CreateTextureHdr( this );

	Int32 HdrPos 				= 1;
	TexturePacker * Child 		= mChild;

	while ( NULL != Child ) {
		TexHdr[ HdrPos ] 	= CreateTextureHdr( Child );
		Child 				= Child->GetChild();
		HdrPos++;
	}

	std::vector<sSubTextureHdr> tSubTexturesHdr;

	std::string path = FileSystem::fileRemoveExtension( mFilepath ) + EE_TEXTURE_ATLAS_EXTENSION;
	IOStreamFile fs ( path , std::ios::out | std::ios::binary );

	if ( fs.isOpen() ) {
		fs.write( reinterpret_cast<const char*> (&TexGrHdr), sizeof(sTextureAtlasHdr) );

		fs.write( reinterpret_cast<const char*> (&TexHdr[ 0 ]), sizeof(sTextureHdr) );

		CreateSubTexturesHdr( this, tSubTexturesHdr );

		if ( tSubTexturesHdr.size() )
			fs.write( reinterpret_cast<const char*> (&tSubTexturesHdr[ 0 ]), sizeof(sSubTextureHdr) * (std::streamsize)tSubTexturesHdr.size() );

		Int32 HdrPos 				= 1;
		TexturePacker * Child 		= mChild;

		while ( NULL != Child ) {
			fs.write( reinterpret_cast<const char*> (&TexHdr[ HdrPos ]), sizeof(sTextureHdr) );

			CreateSubTexturesHdr( Child, tSubTexturesHdr );

			if ( tSubTexturesHdr.size() )
				fs.write( reinterpret_cast<const char*> (&tSubTexturesHdr[ 0 ]), sizeof(sSubTextureHdr) * (std::streamsize)tSubTexturesHdr.size() );

			Child 				= Child->GetChild();

			HdrPos++;
		}
	}
}

void TexturePacker::CreateSubTexturesHdr( TexturePacker * Packer, std::vector<sSubTextureHdr>& SubTextures ) {
	SubTextures.clear();

	sSubTextureHdr tSubTextureHdr;
	Uint32 c = 0;

	std::list<TexturePackerTex*> tTextures = *(Packer->GetTexturePackPtr());
	std::list<TexturePackerTex*>::iterator it;
	TexturePackerTex * tTex;

	SubTextures.resize( tTextures.size() );

	for ( it = tTextures.begin(); it != tTextures.end(); it++ ) {
		tTex = (*it);

		if ( tTex->Placed() ) {
			std::string name = FileSystem::fileNameFromPath( tTex->Name() );

			memset( tSubTextureHdr.Name, 0, HDR_NAME_SIZE );

			String::strCopy( tSubTextureHdr.Name, name.c_str(), HDR_NAME_SIZE );

			if ( !mSaveExtensions )
				name = FileSystem::fileRemoveExtension( name );

			tSubTextureHdr.ResourceID	= String::hash( name );
			tSubTextureHdr.Width 		= tTex->Width();
			tSubTextureHdr.Height 		= tTex->Height();
			tSubTextureHdr.Channels		= tTex->Channels();
			tSubTextureHdr.DestWidth 	= tTex->Width();
			tSubTextureHdr.DestHeight 	= tTex->Height();
			tSubTextureHdr.OffsetX		= 0;
			tSubTextureHdr.OffsetY		= 0;
			tSubTextureHdr.X			= tTex->X();
			tSubTextureHdr.Y			= tTex->Y();
			tSubTextureHdr.Date			= FileSystem::fileGetModificationDate( tTex->Name() );
			tSubTextureHdr.Flags		= 0;

			if ( tTex->Flipped() )
				tSubTextureHdr.Flags |= HDR_SUBTEXTURE_FLAG_FLIPED;

			SubTextures[c] = tSubTextureHdr;

			c++;
		}
	}
}

sTextureHdr	TexturePacker::CreateTextureHdr( TexturePacker * Packer ) {
	sTextureHdr TexHdr;

	std::string name( FileSystem::fileNameFromPath( Packer->GetFilepath() ) );

	memset( TexHdr.Name, 0, HDR_NAME_SIZE );

	String::strCopy( TexHdr.Name, name.c_str(), HDR_NAME_SIZE );

	TexHdr.ResourceID 	= String::hash( name );
	TexHdr.Size			= FileSystem::fileSize( Packer->GetFilepath() );
	TexHdr.SubTextureCount 	= Packer->GetPlacedCount();

	return TexHdr;
}

void TexturePacker::ChildSave( const EE_SAVE_TYPE& Format ) {
	if ( NULL != mChild ) {
		TexturePacker * Parent 	= mChild->GetParent();
		TexturePacker * LastParent	= NULL;
		Int32 ParentCount 			= 0;

		// Find the grand parent
		while ( NULL != Parent ) {
			ParentCount++;
			LastParent = Parent;
			Parent 	= Parent->GetParent();
		}

		if ( NULL != LastParent ) {
			std::string fFpath	= FileSystem::fileRemoveExtension( LastParent->GetFilepath() );
			std::string fExt	= FileSystem::fileExtension( LastParent->GetFilepath() );
			std::string fName	= fFpath + "_ch" + String::toStr( ParentCount ) + "." + fExt;

			mChild->Save( fName, Format, mSaveExtensions );
		}
	}
}

TexturePacker * TexturePacker::GetChild() const {
	return mChild;
}

TexturePacker * TexturePacker::GetParent() const {
	return mParent;
}

std::list<TexturePackerTex*> * TexturePacker::GetTexturePackPtr() {
	return &mTextures;
}

const std::string&	TexturePacker::GetFilepath() const {
	return mFilepath;
}

const Int32& TexturePacker::GetWidth() const {
	return mWidth;
}

const Int32& TexturePacker::GetHeight() const {
	return mHeight;
}

const Int32& TexturePacker::GetPlacedCount() const {
	return mPlacedCount;
}

const Int32& TexturePacker::Width() const {
	return mWidth;
}

const Int32& TexturePacker::Height() const {
	return mHeight;
}

}}
