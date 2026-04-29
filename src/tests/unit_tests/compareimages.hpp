#include "utest.h"

#include <iostream>

#include <eepp/graphics/image.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::Graphics;
using namespace EE::Window;

static void compareImages( utest_state_s& utest_state, int* utest_result, EE::Window::Window* win,
						   const std::string& imageName,
						   const std::string& imagesFolder = "fontrendering",
						   int allowedNumDifferentPixels = 0 ) {
	auto saveType = Image::SaveType::WEBP;
	auto saveExt( Image::saveTypeToExtension( saveType ) );
	std::string expectedImagePath( "assets/" + imagesFolder + "/" + imageName + "." + saveExt );

	Image::FormatConfiguration fconf;
	fconf.webpSaveLossless( true );

	Image actualImage = win->getFrontBufferImage();
	actualImage.setImageFormatConfiguration( fconf );

	if ( !FileSystem::fileExists( expectedImagePath ) )
		actualImage.saveToFile( expectedImagePath, saveType );

	Image expectedImage( expectedImagePath );
	ASSERT_TRUE( expectedImage.getPixelsPtr() != nullptr );
	EXPECT_EQ_MSG( expectedImage.getWidth(), actualImage.getWidth(), "Images width not equal" );
	EXPECT_EQ_MSG( expectedImage.getHeight(), actualImage.getHeight(), "Images height not equal" );

	Image::DiffResult result = actualImage.diff( expectedImage );
	EXPECT_LE( result.numDifferentPixels, allowedNumDifferentPixels );
	if ( result.numDifferentPixels > allowedNumDifferentPixels ) {
		auto saveExt( Image::saveTypeToExtension( saveType ) );
		std::string withTextShaper =
			Text::TextShaperEnabled
				? ( Text::TextShaperOptimizations ? "_text_shape_no_opt" : "_text_shape" )
				: "";
		std::cerr << "Test FAILED: " << result.numDifferentPixels << " pixels differ." << std::endl;
		std::cerr << "Maximum perceptual difference (Delta E): " << result.maxDeltaE << std::endl;
		if ( !FileSystem::fileExists( "output" ) )
			FileSystem::makeDir( "output" );
		std::string actualImagePath =
			"output/" + imageName + "_actual_output" + withTextShaper + "." + saveExt;
		actualImage.saveToFile( actualImagePath, saveType );
		std::cerr << "Actual image saved to: " << actualImagePath << std::endl;
		if ( result.diffImage ) {
			std::string diffImagePath =
				"output/" + imageName + "_diff_output" + withTextShaper + "." + saveExt;
			result.diffImage->setImageFormatConfiguration( fconf );
			result.diffImage->saveToFile( diffImagePath, saveType );
			std::cerr << "Visual diff saved to: " << diffImagePath << std::endl;
		}
	}
}
