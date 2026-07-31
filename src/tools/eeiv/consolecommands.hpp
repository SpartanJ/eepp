#pragma once

#include <eepp/core.hpp>
#include <eepp/graphics/image.hpp>

class App;
namespace EE { namespace UI {
class UIConsole;
}} // namespace EE::UI

using namespace EE;
using namespace EE::Graphics;
using namespace EE::UI;

class ConsoleCommands {
  public:
	ConsoleCommands( App* app, UIConsole* con ) : mApp( app ), Con( con ) {}

	void cmdLoadDir( const std::vector<String>& params );
	void cmdLoadImg( const std::vector<String>& params );
	void cmdSetBackColor( const std::vector<String>& params );
	void cmdSetImgFade( const std::vector<String>& params );
	void cmdSetLateLoading( const std::vector<String>& params );
	void cmdSetBlockWheel( const std::vector<String>& params );
	void cmdMoveTo( const std::vector<String>& params );
	void cmdBatchImgScale( const std::vector<String>& params );
	void cmdBatchImgChangeFormat( const std::vector<String>& params );
	void cmdBatchImgThumbnail( const std::vector<String>& params );
	void cmdImgChangeFormat( const std::vector<String>& params );
	void cmdImgResize( const std::vector<String>& params );
	void cmdImgScale( const std::vector<String>& params );
	void cmdImgThumbnail( const std::vector<String>& params );
	void cmdImgCenterCrop( const std::vector<String>& params );
	void cmdSlideShow( const std::vector<String>& params );
	void cmdSetZoom( const std::vector<String>& params );

  protected:
	App* mApp{ nullptr };
	UIConsole* Con;

	Image::SaveType getPathSaveType( const std::string& path );

	void batchImgScale( const std::string& Path, const Float& Scale, const bool& overridePath );

	void scaleImg( const std::string& Path, const Float& Scale, const bool& overridePath,
				   Image::SaveType saveType = Image::SaveType::Unknown );

	void resizeImg( const std::string& Path, const std::string& outputPath, const Uint32& NewWidth,
					const Uint32& NewHeight, Image::SaveType saveType = Image::SaveType::Unknown );

	void thumbnailImg( const std::string& Path, const Uint32& MaxWidth, const Uint32& MaxHeight,
					   Image::SaveType saveType = Image::SaveType::Unknown );

	void centerCropImg( const std::string& Path, const Uint32& Width, const Uint32& Height,
						Image::SaveType saveType = Image::SaveType::Unknown );

	void batchImgThumbnail( Sizei size, std::string dir, bool recursive );
};
