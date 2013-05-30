#ifndef EE_GRAPHICSCTEXTUREATLAS
#define EE_GRAPHICSCTEXTUREATLAS

/*!
**
** Copyright (c) 2009 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/*! NOTE by Martin Lucas Golini
**	This implementation is based on the John W. Ratcliff texture atlas implementation.
** Implementation differ from the original, but i use the base texture atlas algorithm.
*/

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/packerhelper.hpp>

namespace EE { namespace Graphics {
class cImage;

namespace Private { class cTexturePackerNode; class cTexturePackerTex; }

using namespace Private;

/** @brief The Texture Packer class is used to create new Texture Atlases.
*	Atlases can be created indicating the texture atlas size and adding textures to the atlases.
*/
class EE_API cTexturePacker {
	public:
		/** Creates a new texture packer ( you will need to call SetOptions before adding any texture or image ). */
		cTexturePacker();

		/** Creates a new texture packer indicating the size of the texture atlas.
		*	@param MaxWidth The maximum width that the texture atlas will use.
		*	@param MaxHeight The maximum height that the texture atlas will use.
		*	@param ForcePowOfTwo Indicates that if the max with and height must be adjusted to fit a power of two texture.
		*	@param PixelBorder Indicates how many pixels will be added to separate one image to another in the texture atlas. Usefull to avoid artifacts when rendered scaled SubTextures. Use at least 1 pixel to separate images if you will scale any SubTexture.
		*	@param AllowFlipping Indicates if the images can be flipped inside the texture atlas. This is not compatible with eepp ( since it can't flip the textures back to the original orientation ). So avoid it for eepp.
		*/
		cTexturePacker( const Uint32& MaxWidth, const Uint32& MaxHeight, const bool& ForcePowOfTwo = true, const Uint32& PixelBorder = 0, const bool& AllowFlipping = false );

		~cTexturePacker();

		/** Adds a image/texture from its path to the texture atlas.
		*	@param TexturePath The image path. */
		bool				AddTexture( const std::string& TexturePath );

		/** Adds a image to the texture atlas. The image instance must remain in memory until the texture atlas is saved. */
		bool				AddImage( cImage * Img, const std::string& Name );

		/** Adds a directory with images. It will try to add all the images inside that directory to the texture atlas.  */
		bool				AddTexturesPath( std::string TexturesPath );

		/** After adding all the images that will be used to create the texture atlas. Packing the textures will generate the texture atlas information ( it will fit the images inside the texture atlas, etc ). */
		Int32				PackTextures();

		/** @brief Save the texture atlas to a file, in the indicated format.
		*	If PackTexture() has not been called, it will be called automatically by the function ( so you don't need to call it ).
		*	@param Filepath The path were it will be saved the new texture atlas.
		*	@param Format The image format of the new texture atlas.
		*	@param SaveExtensions Indicates if the extensions of the image files must be saved. Usually you wan't to find the SubTextures by its name without extension, but this can be changed here.
		*/
		void				Save( const std::string& Filepath, const EE_SAVE_TYPE& Format = SAVE_TYPE_PNG, const bool& SaveExtensions = false );

		void				Close();

		/** First of all you need to set at least the max dimensions of the texture atlas.
		*	If the instance of the texture packer was created without indicating this data, this must be called before adding any texture or image.
		*	@param MaxWidth The maximum width that the texture atlas will use.
		*	@param MaxHeight The maximum height that the texture atlas will use.
		*	@param ForcePowOfTwo Indicates that if the max with and height must be adjusted to fit a power of two texture.
		*	@param PixelBorder Indicates how many pixels will be added to separate one image to another in the texture atlas. Usefull to avoid artifacts when rendered scaled SubTextures. Use at least 1 pixel to separate images if you will scale any SubTexture.
		*	@param AllowFlipping Indicates if the images can be flipped inside the texture atlas. This is not compatible with eepp ( since it can't flip the textures back to the original orientation ). So avoid it for eepp.
		*/
		void				SetOptions( const Uint32& MaxWidth, const Uint32& MaxHeight, const bool& ForcePowOfTwo = true, const Uint32& PixelBorder = 0, const bool& AllowFlipping = false );

		/** @return The texture atlas to generate width. */
		const Int32&		Width() const;

		/** @return The texture atlas to generate height */
		const Int32&		Height() const;

		/** @return If the texture atlas has already been saved, returns the file path to the texture atlas. */
		const std::string&	GetFilepath() const;
	protected:
		enum PackStrategy {
			PackBig,
			PackTiny,
			PackFail
		};

		std::list<cTexturePackerTex*>	mTextures;

    	Int32 							mLongestEdge;
    	Int32							mTotalArea;
		cTexturePackerNode *			mFreeList;
    	Int32							mWidth;
    	Int32							mHeight;
		bool							mPacked;
		bool							mAllowFlipping;
		cTexturePacker * 				mChild;
		Int32							mStrategy;
		Int32							mCount;
		cTexturePacker * 				mParent;
		std::string						mFilepath;
		Int32							mPlacedCount;
		bool							mForcePowOfTwo;
		Int32							mPixelBorder;
		bool							mSaveExtensions;
		EE_SAVE_TYPE					mFormat;

		cTexturePacker * 				GetChild() const;

		cTexturePacker * 				GetParent() const;

		std::list<cTexturePackerTex*> *	GetTexturePackPtr();

		void							ChildSave( const EE_SAVE_TYPE& Format );

		void							SaveSubTextures();

    	void 							NewFree( Int32 x, Int32 y, Int32 width, Int32 height );

    	bool 							MergeNodes();

    	void 							Validate();

		cTexturePackerTex *				GetLonguestEdge();

		cTexturePackerTex *				GetShortestEdge();

    	Int32							GetChildCount();

    	const Int32&					GetWidth() const;

    	const Int32&					GetHeight() const;

    	const Int32&					GetPlacedCount() const;

		sTextureHdr						CreateTextureHdr( cTexturePacker * Packer );

		void							CreateSubTexturesHdr( cTexturePacker * Packer, std::vector<sSubTextureHdr>& SubTextures );

		cTexturePackerNode *			GetBestFit( cTexturePackerTex * t, cTexturePackerNode ** prevBestFit, Int32 * EdgeCount );

    	void							InsertTexture( cTexturePackerTex * t, cTexturePackerNode * bestFit, Int32 edgeCount, cTexturePackerNode * previousBestFit );

    	void							AddBorderToTextures( const Int32& BorderSize );

    	void							CreateChild();

		bool							AddPackerText( cTexturePackerTex * TPack );
};

}}

#endif
