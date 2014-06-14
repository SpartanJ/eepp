#ifndef EE_GRAPHICSCIMAGE_HPP
#define EE_GRAPHICSCIMAGE_HPP

#include <eepp/core.hpp>

#include <eepp/math/size.hpp>
#include <eepp/math/rect.hpp>
using namespace EE::Math;

#include <eepp/system/colors.hpp>
using namespace EE::System;

#include <eepp/graphics/renders.hpp>

namespace EE { namespace System {
class Pack;
}}

namespace EE { namespace Graphics {

/** @brief A simple image class to manipulate them. */
class EE_API cImage {
	public:
		/** @return The current Jpeg save quality */
		static Uint32 JpegQuality();

		/** Set the save quality of Jpeg files ( between 0 and 100 )  */
		static void JpegQuality( Uint32 level );

		/** @return The File Extension of a Save Type */
		static std::string SaveTypeToExtension( const Int32& Format );

		/** @return The save type from a given extension ( example: "png" => SAVE_TYPE_PNG ) */
		static EE_SAVE_TYPE ExtensionToSaveType( const std::string& Extension );

		/** @return Convert the number of channels to a pixel format */
		static EE_PIXEL_FORMAT ChannelsToPixelFormat( const Uint32& channels );

		/** @return True if success to get the info.
		* @param path the image path
		* @param width the var to store the image width
		* @param height the var to store the image height
		* @param channels the var to store the image channels count
		*/
		static bool GetInfo( const std::string& path, int * width, int * height, int * channels );

		/** @return True if the file is a valid image
		* @param path the image path
		*/
		static bool IsImage( const std::string& path );

		/** @return The last failure image loading/info reason */
		static std::string GetLastFailureReason();

		cImage();

		/** Copy a image data to create the new image */
		cImage( cImage * image );

		/** Use an existing image ( and appropriates the data passed ) */
		cImage( Uint8* data, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels );

		/** Copy a image data to create the image */
		cImage( const Uint8* data, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels );

		/** Create an empty image */
		cImage( const Uint32& Width, const Uint32& Height, const Uint32& Channels, const ColorA& DefaultColor = ColorA(0,0,0,0), const bool& initWithDefaultColor = true );

		/** Load an image from path
		* @param Path The path to the file.
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		*/
		cImage( std::string Path, const unsigned int& forceChannels = 0 );

		/** Load an image from pack
		* @param Pack The pack file to use to load the image.
		* @param FilePackPath The path of the file inside the pack file.
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		*/
		cImage( Pack * Pack, std::string FilePackPath, const unsigned int& forceChannels = 0 );

		virtual ~cImage();

		/** Create an empty image data */
		void Create( const Uint32& Width, const Uint32& Height, const Uint32& Channels, const ColorA &DefaultColor = ColorA(0,0,0,0), const bool& initWithDefaultColor = true );

		/** Return the pixel color from the image. \n You must have a copy of the image on local memory. For that you need to Lock the image first. */
		virtual ColorA GetPixel(const unsigned int& x, const unsigned int& y);

		/** Set the pixel color to the image. \n You must have a copy of the image on local memory. For that you need to Lock the image first. */
		virtual void SetPixel(const unsigned int& x, const unsigned int& y, const ColorA& Color);

		/** Assign a new array of pixels to the image in local memory ( it has to be exactly of the same size of the image ) */
		virtual void SetPixels( const Uint8* data );

		/** @return A pointer to the first pixel of the image. */
		virtual const Uint8* GetPixelsPtr();

		/** Return the pointer to the array containing the image */
		Uint8 * GetPixels() const;

		/** Set the image Width */
		void Width( const unsigned int& width );

		/** @return The image Width */
		unsigned int Width() const;

		/** Set the image Height */
		void Height( const unsigned int& height );

		/** @return The image Height */
		unsigned int Height() const;

		/** @return The number of channels used by the image */
		unsigned int Channels() const;

		/** Set the number of channels of the image */
		void Channels( const unsigned int& channels );

		/** Clears the current image cache if exists */
		void ClearCache();

		/** @return The Image Size on Memory (in bytes) */
		unsigned int MemSize() const;

		/** @return The image dimensions */
		Sizei Size();

		/** Save the Image to a new File in a specific format */
		virtual bool SaveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format );

		/** Create an Alpha mask from a Color */
		virtual void CreateMaskFromColor( const ColorA& ColorKey, Uint8 Alpha );

		/** Create an Alpha mask from a Color */
		void CreateMaskFromColor( const RGB& ColorKey, Uint8 Alpha );

		/** Replace a color on the image */
		virtual void ReplaceColor( const ColorA& ColorKey, const ColorA& NewColor );

		/** Fill the image with a color */
		virtual void FillWithColor( const ColorA& Color );

		/** Copy the image to this image data, starting from the position x,y */
		virtual void CopyImage( cImage * image, const Uint32& x = 0, const Uint32& y = 0 );

		/** Scale the image */
		virtual void Scale( const Float& scale, EE_RESAMPLER_FILTER filter = RESAMPLER_LANCZOS4 );

		/** Resize the image */
		virtual void Resize( const Uint32& newWidth, const Uint32& newHeight, EE_RESAMPLER_FILTER filter = RESAMPLER_LANCZOS4 );

		/** Flip the image ( rotate the image 90º ) */
		virtual void Flip();

		/** Create a thumnail of the image */
		cImage * Thumbnail( const Uint32& maxWidth, const Uint32& maxHeight, EE_RESAMPLER_FILTER filter = RESAMPLER_LANCZOS4 );

		/** Creates a cropped image from the current image */
		cImage * Crop( Recti rect );

		/** Set as true if you dont want to free the image data in the cImage destruction ( false as default ). */
		void AvoidFreeImage( const bool& AvoidFree );

		/** Blit the image passed onto the current image
		**	@param image The source image to blit onto the image
		**	@param x The x position to start drawing the image
		**	@param y The y position to start drawing the image */
		void Blit( cImage * image, const Uint32& x = 0, const Uint32& y = 0 );

		/** @return A copy of the original image */
		cImage * Copy();

		/** Overload the assigment operator to ensure the image copy */
		cImage& operator =(const cImage& right);
	protected:
		static Uint32 sJpegQuality;

		Uint8 *			mPixels;
		unsigned int 			mWidth;
		unsigned int 			mHeight;
		unsigned int 			mChannels;
		Uint32			mSize;
		bool			mAvoidFree;
		bool			mLoadedFromStbi;

		void 			Allocate( const Uint32& size, ColorA DefaultColor = ColorA(0,0,0,0), bool memsetData = true );

		void			LoadFromPack( Pack * Pack, const std::string& FilePackPath );
};

}}

#endif
