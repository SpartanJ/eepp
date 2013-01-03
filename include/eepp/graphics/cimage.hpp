#ifndef EE_GRAPHICSCIMAGE_HPP
#define EE_GRAPHICSCIMAGE_HPP

#include <eepp/base.hpp>

#include <eepp/math/size.hpp>
using namespace EE::Math;

#include <eepp/system/colors.hpp>
#include <eepp/system/clog.hpp>
#include <eepp/system/cpack.hpp>
#include <eepp/system/cpackmanager.hpp>
using namespace EE::System;

#include <eepp/graphics/renders.hpp>

namespace EE { namespace Graphics {

class EE_API cImage {
	public:
		/** @return The current Jpeg save quality */
		static Uint32 JpegQuality();

		/** Set the save quality of Jpeg files ( between 0 and 100 )  */
		static void JpegQuality( Uint32 level );

		/** @return The File Extension of a Save Type */
		static std::string SaveTypeToExtension( const Int32& Format );

		/** @return The save type from a given extension ( example: "png" => EE_SAVE_TYPE_PNG ) */
		static EE_SAVE_TYPE ExtensionToSaveType( const std::string& Extension );

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

		/** Use an existing image */
		cImage( Uint8* data, const eeUint& Width, const eeUint& Height, const eeUint& Channels );

		/** Copy a image data to create the image */
		cImage( const Uint8* data, const eeUint& Width, const eeUint& Height, const eeUint& Channels );

		/** Create an empty image */
		cImage( const Uint32& Width, const Uint32& Height, const Uint32& Channels, const eeColorA& DefaultColor = eeColorA(0,0,0,0) );

		/** Load an image from path */
		cImage( std::string Path );

		/** Load an image from pack */
		cImage( cPack * Pack, std::string FilePackPath );

		virtual ~cImage();

		/** Create an empty image data */
		void Create(const Uint32& Width, const Uint32& Height, const Uint32& Channels, const eeColorA &DefaultColor = eeColorA(0,0,0,0) );

		/** Return the pixel color from the image. \n You must have a copy of the image on local memory. For that you need to Lock the image first. */
		virtual eeColorA GetPixel(const eeUint& x, const eeUint& y);

		/** Set the pixel color to the image. \n You must have a copy of the image on local memory. For that you need to Lock the image first. */
		virtual void SetPixel(const eeUint& x, const eeUint& y, const eeColorA& Color);

		/** Assign a new array of pixels to the image in local memory ( it has to be exactly of the same size of the image ) */
		virtual void SetPixels( const Uint8* data );

		/** @return A pointer to the first pixel of the image. */
		virtual const Uint8* GetPixelsPtr();

		/** Return the pointer to the array containing the image */
		Uint8 * GetPixels() const;

		/** Set the image Width */
		void Width( const eeUint& width );

		/** @return The image Width */
		eeUint Width() const;

		/** Set the image Height */
		void Height( const eeUint& height );

		/** @return The image Height */
		eeUint Height() const;

		/** @return The number of channels used by the image */
		eeUint Channels() const;

		/** Set the number of channels of the image */
		void Channels( const eeUint& channels );

		/** Clears the current image cache if exists */
		void ClearCache();

		/** @return The Image Size on Memory (in bytes) */
		eeUint MemSize() const;

		/** @return The image dimensions */
		eeSize Size();

		/** Save the Image to a new File in a specific format */
		virtual bool SaveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format );

		/** Create an Alpha mask from a Color */
		virtual void CreateMaskFromColor( const eeColorA& ColorKey, Uint8 Alpha );

		/** Create an Alpha mask from a Color */
		void CreateMaskFromColor( const eeColor& ColorKey, Uint8 Alpha );

		/** Replace a color on the image */
		virtual void ReplaceColor( const eeColorA& ColorKey, const eeColorA& NewColor );

		/** Fill the image with a color */
		virtual void FillWithColor( const eeColorA& Color );

		/** Copy the image to this image data, starting from the position x,y */
		virtual void CopyImage( cImage * Img, const eeUint& x, const eeUint& y );

		/** Scale the image */
		virtual void Scale( const eeFloat& scale );

		/** Resize the image */
		virtual void Resize( const eeUint& new_width, const eeUint& new_height );

		/** Flip the image ( rotate the image 90º ) */
		virtual void Flip();

		/** Create a thumnail of the image */
		cImage * Thumbnail( const eeUint& max_width, const eeUint& max_height );

		/** Set as true if you dont want to free the image data ( false as default ). */
		void AvoidFreeImage( const bool& AvoidFree ) { mAvoidFree = AvoidFree; }
	protected:
		static Uint32 sJpegQuality;

		Uint8 *			mPixels;
		eeUint 			mWidth;
		eeUint 			mHeight;
		eeUint 			mChannels;
		Uint32			mSize;
		bool			mAvoidFree;
		bool			mLoadedFromStbi;

		void 			Allocate( const Uint32& size, eeColorA DefaultColor = eeColorA(0,0,0,0) );

		void			LoadFromPack( cPack * Pack, const std::string& FilePackPath );
};

}}

#endif
