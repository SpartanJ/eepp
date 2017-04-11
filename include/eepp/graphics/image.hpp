#ifndef EE_GRAPHICSCIMAGE_HPP
#define EE_GRAPHICSCIMAGE_HPP

#include <eepp/core.hpp>

#include <eepp/math/size.hpp>
#include <eepp/math/rect.hpp>
using namespace EE::Math;

#include <eepp/system/color.hpp>
using namespace EE::System;

#include <eepp/graphics/graphicshelper.hpp>

namespace EE { namespace System {
class Pack;
}}

namespace EE { namespace Graphics {

/** @brief A simple image class to manipulate them. */
class EE_API Image {
	public:
		/** @return The current Jpeg save quality */
		static Uint32 jpegQuality();

		/** Set the save quality of Jpeg files ( between 0 and 100 )  */
		static void jpegQuality( Uint32 level );

		/** @return The File Extension of a Save Type */
		static std::string saveTypeToExtension( const Int32& Format );

		/** @return The save type from a given extension ( example: "png" => SAVE_TYPE_PNG ) */
		static EE_SAVE_TYPE extensionToSaveType( const std::string& Extension );

		/** @return Convert the number of channels to a pixel format */
		static EE_PIXEL_FORMAT channelsToPixelFormat( const Uint32& channels );

		/** @return True if success to get the info.
		* @param path the image path
		* @param width the var to store the image width
		* @param height the var to store the image height
		* @param channels the var to store the image channels count
		*/
		static bool getInfo( const std::string& path, int * width, int * height, int * channels );

		/** @return True if the file is a valid image
		* @param path the image path
		*/
		static bool isImage( const std::string& path );

		/** @return The last failure image loading/info reason */
		static std::string getLastFailureReason();

		Image();

		/** Copy a image data to create the new image */
		Image( Graphics::Image * image );

		/** Use an existing image ( and appropriates the data passed ) */
		Image( Uint8* data, const unsigned int& width, const unsigned int& height, const unsigned int& channels );

		/** Copy a image data to create the image */
		Image( const Uint8* data, const unsigned int& width, const unsigned int& height, const unsigned int& channels );

		/** Create an empty image */
		Image( const Uint32& width, const Uint32& height, const Uint32& channels, const Color& DefaultColor = Color(0,0,0,0), const bool& initWithDefaultColor = true );

		/** Load an image from path
		* @param Path The path to the file.
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		*/
		Image( std::string Path, const unsigned int& forceChannels = 0 );

		/** Load an image from pack
		* @param Pack The pack file to use to load the image.
		* @param FilePackPath The path of the file inside the pack file.
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		*/
		Image( Pack * Pack, std::string FilePackPath, const unsigned int& forceChannels = 0 );

		virtual ~Image();

		/** Create an empty image data */
		void create( const Uint32& width, const Uint32& height, const Uint32& channels, const Color &DefaultColor = Color(0,0,0,0), const bool& initWithDefaultColor = true );

		/** Return the pixel color from the image. \n You must have a copy of the image on local memory. For that you need to Lock the image first. */
		virtual Color getPixel(const unsigned int& x, const unsigned int& y);

		/** Set the pixel color to the image. \n You must have a copy of the image on local memory. For that you need to Lock the image first. */
		virtual void setPixel(const unsigned int& x, const unsigned int& y, const Color& Color);

		/** Assign a new array of pixels to the image in local memory ( it has to be exactly of the same size of the image ) */
		virtual void setPixels( const Uint8* data );

		/** @return A pointer to the first pixel of the image. */
		virtual const Uint8* getPixelsPtr();

		/** Return the pointer to the array containing the image */
		Uint8 * getPixels() const;

		/** Set the image Width */
		void setWidth( const unsigned int& width );

		/** @return The image Width */
		unsigned int getWidth() const;

		/** Set the image Height */
		void setHeight( const unsigned int& height );

		/** @return The image Height */
		unsigned int getHeight() const;

		/** @return The number of channels used by the image */
		unsigned int getChannels() const;

		/** Set the number of channels of the image */
		void setChannels( const unsigned int& setChannels );

		/** Clears the current image cache if exists */
		void clearCache();

		/** @return The Image Size on Memory (in bytes) */
		unsigned int getMemSize() const;

		/** @return The image dimensions */
		Sizei getSize();

		/** Save the Image to a new File in a specific format */
		virtual bool saveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format );

		/** Create an Alpha mask from a Color */
		virtual void createMaskFromColor( const Color& ColorKey, Uint8 Alpha );

		/** Create an Alpha mask from a Color */
		void createMaskFromColor( const RGB& ColorKey, Uint8 Alpha );

		/** Replace a color on the image */
		virtual void replaceColor( const Color& ColorKey, const Color& NewColor );

		/** Fill the image with a color */
		virtual void fillWithColor( const Color& Color );

		/** Copy the image to this image data, starting from the position x,y */
		virtual void copyImage( Graphics::Image * image, const Uint32& x = 0, const Uint32& y = 0 );

		/** Scale the image */
		virtual void scale( const Float& scale, EE_RESAMPLER_FILTER filter = RESAMPLER_LANCZOS4 );

		/** Resize the image */
		virtual void resize( const Uint32& newWidth, const Uint32& newHeight, EE_RESAMPLER_FILTER filter = RESAMPLER_LANCZOS4 );

		/** Flip the image ( rotate the image 90º ) */
		virtual void flip();

		/** Create a thumnail of the image */
		Graphics::Image * thumbnail( const Uint32& maxWidth, const Uint32& maxHeight, EE_RESAMPLER_FILTER filter = RESAMPLER_LANCZOS4 );

		/** Creates a cropped image from the current image */
		Graphics::Image * crop( Recti rect );

		/** Set as true if you dont want to free the image data in the Image destruction ( false as default ). */
		void avoidFreeImage( const bool& AvoidFree );

		/** Blit the image passed onto the current image
		**	@param image The source image to blit onto the image
		**	@param x The x position to start drawing the image
		**	@param y The y position to start drawing the image */
		void blit( Graphics::Image * image, const Uint32& x = 0, const Uint32& y = 0 );

		/** @return A copy of the original image */
		Graphics::Image * copy();

		/** Overload the assigment operator to ensure the image copy */
		Graphics::Image& operator =(const Image& right);
	protected:
		static Uint32 sJpegQuality;

		Uint8 *			mPixels;
		unsigned int 	mWidth;
		unsigned int 	mHeight;
		unsigned int 	mChannels;
		Uint32			mSize;
		bool			mAvoidFree;
		bool			mLoadedFromStbi;

		void 			allocate( const Uint32& size, Color DefaultColor = Color(0,0,0,0), bool memsetData = true );

		void			loadFromPack( Pack * Pack, const std::string& FilePackPath );
};

}}

#endif
