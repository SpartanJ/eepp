#include <args/args.hxx>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/system/filesystem.hpp>
#include <iostream>
#include <map>
#include <memory>

using namespace EE;
using namespace EE::System;
using namespace EE::Graphics;

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	args::ArgumentParser parser( "Texture Packer - eepp texture atlas creator." );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::ValueFlag<std::string> texturesPath( parser, "textures-path", "Textures directory path.",
											   { 'p', "textures-path" } );
	args::ValueFlagList<std::string> images( parser, "image-path", "Input image path.",
											 { 'i', "image-path" } );
	args::ValueFlag<std::string> outputFile(
		parser, "output-file", "Texture atlas file output path. Extension must be: \".eta\"",
		{ 'o', "output-file" }, "", args::Options::Required | args::Options::Single );
	std::unordered_map<std::string, Image::SaveType> saveTypeFormat{
		{ "PNG", Image::SaveType::SAVE_TYPE_PNG }, { "DDS", Image::SaveType::SAVE_TYPE_DDS },
		{ "TGA", Image::SaveType::SAVE_TYPE_TGA }, { "BMP", Image::SaveType::SAVE_TYPE_BMP },
		{ "JPG", Image::SaveType::SAVE_TYPE_JPG }, { "QOI", Image::SaveType::SAVE_TYPE_QOI } };
	args::MapFlag<std::string, Image::SaveType> saveType(
		parser, "image-format", "Output image format.", { 'f', "image-format" }, saveTypeFormat,
		Image::SaveType::SAVE_TYPE_PNG, args::Options::Single );
	std::unordered_map<std::string, PixelDensitySize> pixelDensityMap{
		{ "MDPI", PixelDensitySize::MDPI },
		{ "HDPI", PixelDensitySize::HDPI },
		{ "XHDPI", PixelDensitySize::XHDPI },
		{ "XXHDPI", PixelDensitySize::XXHDPI },
		{ "XXXHDPI", PixelDensitySize::XXXHDPI } };
	args::MapFlag<std::string, PixelDensitySize> pixelDensity(
		parser, "pixel-density",
		"Source images pixel density size. Valid values are: MDPI (1dp = 1px), HDPI (1dp = 1.5px), "
		"XHDPI (1dp = 2px), XXHDPI (1dp = 3px) and XXXHDPI (1dp = 4px).",
		{ 'd', "pixel-density" }, pixelDensityMap, PixelDensitySize::MDPI, args::Options::Single );
	args::Flag forcePow2( parser, "force-power-of-two-texture", "Force power of two texture.",
						  { "force-power-of-two" }, args::Options::Single );
	args::Flag scalableSVG( parser, "scalable-svg",
							"Scale SVG source files using the pixel-density provided.",
							{ "scalable-svg" }, args::Options::Single );
	args::Flag saveExtensions( parser, "save-extensions",
							   "Save the file extensions as part of the texture regions names.",
							   { "save-extensions" } );
	args::Flag allowChilds(
		parser, "allow-childs",
		"When enabled in the case of an atlas not having enough space in the image to fit all the "
		"source input images it will create new child atlas images to save them.",
		{ "allow-childs" } );
	args::ValueFlag<Uint32> height( parser, "max-width", "Texture Atlas maximum allowed height.",
									{ 'h', "max-height" }, 4096, args::Options::Single );
	args::ValueFlag<Uint32> width( parser, "max-width", "Texture Atlas maximum allowed width.",
								   { 'w', "max-width" }, 4096, args::Options::Single );
	args::ValueFlag<Uint32> pixelsBorder(
		parser, "pixels-border",
		"Number of pixels used as border of each image. The border is the separator between images "
		"and it's recommended that at least there's a 2 pixel border (default value) to avoid "
		"rendering problems.",
		{ 'b', "pixels-border" }, 2, args::Options::Single );
	args::Flag update( parser, "update", "Update texture atlas if output file already exists.",
					   { 'u', "update" }, args::Options::Single );
	std::unordered_map<std::string, Texture::Filter> textureFilterMap{
		{ "linear", Texture::Filter::Linear }, { "nearest", Texture::Filter::Nearest } };
	args::MapFlag<std::string, Texture::Filter> textureFilter(
		parser, "texture-filter",
		"Texture filter to use with the texture atlas. Available filters: \"linear\" or "
		"\"nearest\".",
		{ "texture-filter" }, textureFilterMap, Texture::Filter::Linear, args::Options::Single );

	try {
		parser.ParseCLI( argc, argv );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	bool hasImages = false;
	auto imagesPaths = args::get( images );
	std::map<std::string, std::unique_ptr<Image>> imagesList;
	if ( !imagesPaths.empty() ) {
		for ( auto image : images ) {
			if ( !Image::isImage( image ) ) {
				std::cout << "Image: \"" << image << "\" is invalid." << std::endl
						  << "Operation cancelled." << std::endl;
				return EXIT_FAILURE;
			}

			if ( imagesList.find( image ) == imagesList.end() ) {
				imagesList[image] = std::make_unique<Image>( image );
			}
		}
		hasImages = true;
	}

	std::string texturesPathSafe( texturesPath.Get() );
	FileSystem::dirAddSlashAtEnd( texturesPathSafe );
	if ( !FileSystem::isDirectory( texturesPathSafe ) && !hasImages ) {
		std::cout << "textures-path is invalid.";
		return EXIT_FAILURE;
	}

	auto filesInPath = FileSystem::filesGetInPath( texturesPathSafe );
	if ( !hasImages ) {
		for ( auto& file : filesInPath ) {
			if ( Image::isImage( texturesPathSafe + file ) ) {
				hasImages = true;
				break;
			}
		}
		if ( !hasImages ) {
			std::cout << "textures-path must contain at least one image.";
			return EXIT_FAILURE;
		}
	}

	if ( pixelsBorder.Get() > 32 ) {
		std::cout << "pixels-border value invalid, try a smaller number." << std::endl;
		return EXIT_FAILURE;
	}

	if ( FileSystem::fileExtension( outputFile.Get() ).empty() ||
		 FileSystem::fileExtension( outputFile.Get() ) != "eta" ) {
		std::cout << "output-file must have an extension.";
		return EXIT_FAILURE;
	}

	if ( !FileSystem::fileExists( outputFile.Get() ) ) {
		TexturePacker tp( width.Get(), height.Get(), PixelDensity::toFloat( pixelDensity.Get() ),
						  forcePow2.Get(), scalableSVG.Get(), pixelsBorder.Get(),
						  textureFilter.Get(), allowChilds.Get() );
		std::cout << "Packing directory: " << texturesPathSafe << std::endl;
		tp.addTexturesPath( texturesPathSafe );
		for ( auto& image : imagesList ) {
			tp.addImage( image.second.get(), image.first );
		}
		if ( tp.packTextures() <= 0 ) {
			goto exit_error;
		}
		std::string outputTexturePath( FileSystem::fileRemoveExtension( outputFile.Get() ) + "." +
									   Image::saveTypeToExtension( saveType.Get() ) );
		tp.save( outputTexturePath, saveType.Get(), saveExtensions.Get() );
		std::cout << "Texture Atlas created." << std::endl;
	} else if ( update.Get() ) {
		TextureAtlasLoader tgl;
		std::cout << "Texture Atlas is already present, updating it." << std::endl;
		if ( !tgl.updateTextureAtlas( outputFile.Get(), texturesPathSafe,
									  Sizei( width, height ) ) ) {
			goto exit_error;
		}
		std::cout << "Texture Atlas updated." << std::endl;
	}
	return EXIT_SUCCESS;
exit_error:
	std::cout << "Texture Atlas creation failed.\nTry allowing bigger images modifying the "
				 "max-width and max-height, or try removing some images from the images "
				 "directory."
			  << std::endl;
	return EXIT_FAILURE;
}
