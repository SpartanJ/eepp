#include "utest.h"

#include <iostream>

#include <eepp/graphics/image.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::Graphics;
using namespace EE::Window;

static constexpr Uint32 VisualTestWindowStyle = WindowStyle::Borderless;

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
	const bool imageSizeMismatch = expectedImage.getWidth() != actualImage.getWidth() ||
								   expectedImage.getHeight() != actualImage.getHeight();
	EXPECT_EQ_MSG( expectedImage.getWidth(), actualImage.getWidth(), "Images width not equal" );
	EXPECT_EQ_MSG( expectedImage.getHeight(), actualImage.getHeight(), "Images height not equal" );

	Image::DiffResult result = actualImage.diff( expectedImage );
	EXPECT_LE( result.numDifferentPixels, allowedNumDifferentPixels );
	if ( imageSizeMismatch || result.numDifferentPixels > allowedNumDifferentPixels ) {
		auto saveExt( Image::saveTypeToExtension( saveType ) );
		std::string withTextShaper =
			Text::TextShaperEnabled
				? ( Text::TextShaperOptimizations ? "_text_shape_no_opt" : "_text_shape" )
				: "";
		Sizei winSize = win->getSize();
		Sizei screenCoordSize = win->getSizeInScreenCoordinates();
		Sizei lastWindowedSize = win->getLastWindowedSize();
		Sizei lastWindowedScreenCoordSize = win->getLastWindowedSizeInScreenCoordinates();
		Sizei desktop = win->getDesktopResolution();
		Vector2i pos = win->getPosition();
		Rect borders = win->getBorderSize();
		const WindowInfo* winInfo = win->getWindowInfo();
		std::cerr << "Window diagnostics for failed image '" << imageName << "':" << std::endl;
		std::cerr << "  OS: " << Sys::getOSName( true ) << " / " << Sys::getOSArchitecture()
				  << std::endl;
		std::cerr << "  Expected image: " << expectedImage.getWidth() << "x"
				  << expectedImage.getHeight() << " from " << expectedImagePath << std::endl;
		std::cerr << "  Actual image: " << actualImage.getWidth() << "x" << actualImage.getHeight()
				  << std::endl;
		std::cerr << "  WindowConfig: " << winInfo->WindowConfig.Width << "x"
				  << winInfo->WindowConfig.Height << " title='" << winInfo->WindowConfig.Title
				  << "' style=0x" << std::hex << winInfo->WindowConfig.Style << std::dec
				  << " disableHiDPI=" << ( winInfo->WindowConfig.DisableHiDPI ? "true" : "false" )
				  << " pixelDensity=" << winInfo->WindowConfig.PixelDensity << std::endl;
		std::cerr << "  Window size: " << winSize.getWidth() << "x" << winSize.getHeight()
				  << " screenCoordinates=" << screenCoordSize.getWidth() << "x"
				  << screenCoordSize.getHeight() << " scale=" << win->getScale() << std::endl;
		std::cerr << "  Last windowed size: " << lastWindowedSize.getWidth() << "x"
				  << lastWindowedSize.getHeight()
				  << " screenCoordinates=" << lastWindowedScreenCoordSize.getWidth() << "x"
				  << lastWindowedScreenCoordSize.getHeight() << std::endl;
		std::cerr << "  Desktop: " << desktop.getWidth() << "x" << desktop.getHeight()
				  << " position=" << pos.x << "," << pos.y
				  << " displayIndex=" << win->getCurrentDisplayIndex() << std::endl;
		std::cerr << "  Borders top,left,bottom,right: " << borders.Top << "," << borders.Left
				  << "," << borders.Bottom << "," << borders.Right << std::endl;
		DisplayManager* displayManager =
			Engine::existsSingleton() ? Engine::instance()->getDisplayManager() : nullptr;
		if ( displayManager ) {
			const int displayCount = displayManager->getDisplayCount();
			std::cerr << "  Displays: " << displayCount << std::endl;
			for ( int i = 0; i < displayCount; i++ ) {
				Display* display = displayManager->getDisplayIndex( i );
				if ( !display )
					continue;
				Rect bounds = display->getBounds();
				Rect usableBounds = display->getUsableBounds();
				DisplayMode currentMode = display->getCurrentMode();
				Sizeu displaySize = display->getSize();
				std::cerr << "    [" << i << "] name='" << display->getName()
						  << "' bounds=" << bounds.Left << "," << bounds.Top << " " << bounds.Right
						  << "x" << bounds.Bottom << " usable=" << usableBounds.Left << ","
						  << usableBounds.Top << " " << usableBounds.Right << "x"
						  << usableBounds.Bottom << " currentMode=" << currentMode.Width << "x"
						  << currentMode.Height << "@" << currentMode.RefreshRate
						  << " size=" << displaySize.getWidth() << "x" << displaySize.getHeight()
						  << " dpi=" << display->getDPI()
						  << " refreshRate=" << display->getRefreshRate() << std::endl;
			}
		}
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
