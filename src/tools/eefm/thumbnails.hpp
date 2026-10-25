#ifndef EEFM_THUMBNAILS_HPP
#define EEFM_THUMBNAILS_HPP

#include <eepp/core/containers.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/imagethumbnailer.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <memory>

namespace EE::UI {

class UITableCell;

}

namespace eefm {

using namespace EE;
using namespace EE::Graphics;
using namespace EE::UI;
using namespace EE::VFS;
class App;
class FileTableView;
struct FileTab;

/** Binds only visible file rows to the reusable CPU service. Owns no tab or cell lifetime. */
class Thumbnails {
  public:
	explicit Thumbnails( App& app );

	~Thumbnails();

	void attach( FileTab* tab );

	void configure();

	DrawablePtr icon( FileTab* tab, const VFSFileInfo& info ) const;

	void clearCache();

	void shutdown();

  private:
	struct View;

	void updateVisible( FileTab* tab, size_t first, size_t end );

	void updateCell( FileTab* tab, UITableCell* cell );

	App& mApp;
	std::unique_ptr<ImageThumbnailer> mService;
	UnorderedMap<FileTableView*, std::shared_ptr<View>> mViews;
	bool mStopped{ false };
};

} // namespace eefm
#endif
