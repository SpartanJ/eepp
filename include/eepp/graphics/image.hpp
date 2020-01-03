#ifndef EE_GRAPHICSCIMAGE_HPP
#define EE_GRAPHICSCIMAGE_HPP

#include <eepp/core.hpp>

#include <eepp/math/size.hpp>
#include <eepp/math/rect.hpp>
using namespace EE::Math;

#include <eepp/system/color.hpp>
using namespace EE::System;

#include <eepp/graphics/rendermode.hpp>

struct NSVGimage;

namespace EE { namespace System {
class Pack;
class IOStream;
}}

namespace EE { namespace Graphics {

/** @brief A simple image class to manipulate them. */
class EE_API Image {
	public:
		/** @enum PixelFormat Format Pixel formats to write into a texture image. */
		enum PixelFormat {
			PIXEL_FORMAT_RED,
			PIXEL_FORMAT_RG,
			PIXEL_FORMAT_RGB,
			PIXEL_FORMAT_BGR,
			PIXEL_FORMAT_RGBA,
			PIXEL_FORMAT_BGRA
		};

		/** @enum ResamplerFilter ºººFilter The filter used to resample/resize an image. */
		enum ResamplerFilter {
			RESAMPLER_BOX,
			RESAMPLER_TENT,
			RESAMPLER_BELL,
			RESAMPLER_BSPLINE,
			RESAMPLER_MITCHELL,
			RESAMPLER_LANCZOS3,
			RESAMPLER_BLACKMAN,
			RESAMPLER_LANCZOS4,
			RESAMPLER_LANCZOS6,
			RESAMPLER_LANCZOS12,
			RESAMPLER_KAISER,
			RESAMPLER_GAUSSIAN,
			RESAMPLER_CATMULLROM,
			RESAMPLER_QUADRATIC_INTERP,
			RESAMPLER_QUADRATIC_APPROX,
			RESAMPLER_QUADRATIC_MIX
		};

		/** @enum SaveType Defines the format to save a texture. */
		enum SaveType {
			SAVE_TYPE_UNKNOWN	= -1,
			SAVE_TYPE_TGA		= 0,
			SAVE_TYPE_BMP		= 1,
			SAVE_TYPE_PNG		= 2,
			SAVE_TYPE_DDS		= 3,
			SAVE_TYPE_JPG		= 4
		};

		class FormatConfiguration
		{
			public:
				FormatConfiguration() :
					mSvgScale(1.f),
					mJpegSaveQuality(85)
				{}

				/** @return The current Jpeg save quality */
				const Uint32& jpegSaveQuality() const { return mJpegSaveQuality; }

				/** Set the save quality of Jpeg files ( between 0 and 100 )  */
				void jpegSaveQuality( Uint32 level ) {
					level = eemin<Uint32>( level, 100 );
					mJpegSaveQuality = level;
				}

				/** @return The current SVG default scale */
				const Float& svgScale() const { return mSvgScale; }

				/** Set the SVG default scale */
				void svgScale( Float scale ) { mSvgScale = scale; }

			protected:
				Float mSvgScale;
				Uint32 mJpegSaveQuality;
		};

		/** @return The File Extension of a Save Type */
		static std::string saveTypeToExtension( const Int32& Format );

		/** @return The save type from a given extension ( example: "png" => SaveType::SAVE_TYPE_PNG ) */
		static SaveType extensionToSaveType( const std::string& Extension );

		/** @return Convert the number of channels to a pixel format */
		static PixelFormat channelsToPixelFormat( const Uint32& channels );

		/** @return True if success to get the info.
		* @param path the image path
		* @param width the var to store the image width
		* @param height the var to store the image height
		* @param channels the var to store the image channels count
		* @param imageFormatConfiguration The specific image format configuration to use when decoding the image.
		*/
		static bool getInfo( const std::string& path, int * width, int * height, int * channels, const FormatConfiguration& imageFormatConfiguration = FormatConfiguration() );

		/** @return True if the file is a valid image ( reads the file header to know if the file is an image file format supported )
		* @param path the image path
		*/
		static bool isImage( const std::string& path );

		/** @return If the path or file name has a supported image file extension
		*   @param path the image path or file name
		*/
		static bool isImageExtension( const std::string& path );

		/** @return The last failure image loading/info reason */
		static std::string getLastFailureReason();

		static Image * New();

		static Image * New( Graphics::Image * image );

		static Image * New( Uint8* data, const unsigned int& width, const unsigned int& height, const unsigned int& channels );

		static Image * New( const Uint8* data, const unsigned int& width, const unsigned int& height, const unsigned int& channels );

		static Image * New( const Uint32& width, const Uint32& height, const Uint32& channels, const Color& DefaultColor = Color(0,0,0,0), const bool& initWithDefaultColor = true );

		static Image * New( std::string Path, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

		static Image * New( const Uint8* imageData, const unsigned int& imageDataSize, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

		static Image * New( Pack * Pack, std::string FilePackPath, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

		static Image * New( IOStream& stream, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

		Image();

		/** Copy a image data to create the new image */
		explicit Image( Graphics::Image * image );

		/** Use an existing image ( and appropriates the data passed ) */
		Image( Uint8* data, const unsigned int& width, const unsigned int& height, const unsigned int& channels );

		/** Copy a image data to create the image */
		Image( const Uint8* data, const unsigned int& width, const unsigned int& height, const unsigned int& channels );

		/** Create an empty image */
		Image( const Uint32& width, const Uint32& height, const Uint32& channels, const Color& DefaultColor = Color(0,0,0,0), const bool& initWithDefaultColor = true );

		/** Load an image from path
		* @param Path The path to the file.
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		* @param formatConfiguration The specific image format configuration to use when decoding the image.
		*/
		Image( std::string Path, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

		/** Load a compressed image from memory
		* @param imageData The image data
		* @param imageDataSize The image size
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		* @param formatConfiguration The specific image format configuration to use when decoding the image.
		*/
		Image( const Uint8* imageData, const unsigned int& imageDataSize, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

		/** Load an image from pack
		* @param Pack The pack file to use to load the image.
		* @param FilePackPath The path of the file inside the pack file.
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		* @param formatConfiguration The specific image format configuration to use when decoding the image.
		*/
		Image( Pack * Pack, std::string FilePackPath, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

		/** Load an image from stream
		* @param stream The stream to read the image
		* @param forceChannels Number of channels to use for the image, default 0 means that it use the default image channels.
		* @param formatConfiguration The specific image format configuration to use when decoding the image.
		*/
		Image( IOStream& stream, const unsigned int& forceChannels = 0, const FormatConfiguration& formatConfiguration = FormatConfiguration() );

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
		virtual bool saveToFile( const std::string& filepath, const SaveType& Format );

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
		virtual void scale( const Float& scale, ResamplerFilter filter = ResamplerFilter::RESAMPLER_LANCZOS4 );

		/** Resize the image */
		virtual void resize(const Uint32& newWidth, const Uint32& newHeight, ResamplerFilter filter = ResamplerFilter::RESAMPLER_LANCZOS4 );

		/** Flip the image ( rotate the image 90º ) */
		virtual void flip();

		/** Create a thumnail of the image */
		Graphics::Image * thumbnail( const Uint32& maxWidth, const Uint32& maxHeight, ResamplerFilter filter = ResamplerFilter::RESAMPLER_LANCZOS4 );

		/** Creates a cropped image from the current image */
		Graphics::Image * crop( Rect rect );

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

		/** Set the image format configuration */
		void setImageFormatConfiguration( const FormatConfiguration& imageFormatConfiguration );

		/** @return The image format configuration */
		const FormatConfiguration& getImageFormatConfiguration() const;
	protected:
		Uint8 *			mPixels;
		unsigned int 	mWidth;
		unsigned int 	mHeight;
		unsigned int 	mChannels;
		Uint32			mSize;
		bool			mAvoidFree;
		bool			mLoadedFromStbi;
		FormatConfiguration mFormatConfiguration;

		void 			allocate( const Uint32& size, Color DefaultColor = Color(0,0,0,0), bool memsetData = true );

		void			loadFromPack( Pack * Pack, const std::string& FilePackPath );

		void			svgLoad( NSVGimage * image );
};

}}

#endif
