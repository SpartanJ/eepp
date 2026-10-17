#include <eepp/vfs/providers/localprovider.hpp>

namespace EE { namespace VFS {

LocalProvider::LocalProvider( std::shared_ptr<LocalIOScheduler> scheduler ) :
	mScheduler( std::move( scheduler ) ) {}

LocalProvider::~LocalProvider() = default;

}} // namespace EE::VFS
