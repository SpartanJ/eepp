#ifndef EE_SYSTEMCPACKMANAGER_HPP
#define EE_SYSTEMCPACKMANAGER_HPP

#include "tsingleton.hpp"
#include "tcontainer.hpp"
#include "cpack.hpp"

namespace EE { namespace System {

class EE_API cPackManager : public tContainer<cPack>, public tSingleton<cPackManager> {
	public:
		SINGLETON_DECLARE_HEADERS(cPackManager)

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
