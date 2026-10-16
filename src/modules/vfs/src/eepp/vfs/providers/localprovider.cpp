#include <eepp/vfs/providers/localprovider.hpp>

namespace EE { namespace VFS {

LocalProvider::LocalProvider( std::shared_ptr<System::ThreadPool> pool ) :
	mPool( std::move( pool ) ) {}

LocalProvider::~LocalProvider() = default;

}} // namespace EE::VFS
