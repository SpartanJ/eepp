#ifndef EE_SYSTEMCPACKMANAGER_HPP
#define EE_SYSTEMCPACKMANAGER_HPP

#include <eepp/system/tsingleton.hpp>
#include <eepp/system/tcontainer.hpp>
#include <eepp/system/cpack.hpp>

namespace EE { namespace System {

class EE_API cPackManager : public tContainer<cPack> {
	SINGLETON_DECLARE_HEADERS(cPackManager)

	public:
		cPackManager();

		virtual ~cPackManager();

		cPack * Exists( std::string& path );

		const bool& FallbackToPacks() const;

		void FallbackToPacks( const bool& fallback );
	protected:
		bool	mFallback;
};

}}

#endif
