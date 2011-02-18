#ifndef EE_WINDOWCCURSORMANAGERNULL_HPP
#define EE_WINDOWCCURSORMANAGERNULL_HPP

#include "../../ccursormanager.hpp"

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace Null {

class cCursorManagerNull : public cCursorManager {
	public:
		cCursorManagerNull( cWindow * window );

		cCursor * Create( cTexture * tex, const eeVector2i& hotspot, const std::string& name );

		cCursor * Create( cImage * img, const eeVector2i& hotspot, const std::string& name );

		cCursor * Create( const std::string& path, const eeVector2i& hotspot, const std::string& name );

		void Set( cCursor * cursor );

		void Set( EE_SYSTEM_CURSOR syscurid );

		void Show();

		void Hide();

		void Visible( bool visible );

		void Reload();
};

}}}}

#endif
