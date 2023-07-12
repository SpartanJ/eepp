#ifndef ECODE_ICONMANAGER_HPP
#define ECODE_ICONMANAGER_HPP

#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/ui/uiscenenode.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::UI;

namespace ecode {

class IconManager {
  public:
	static void init( UISceneNode* sceneNode, FontTrueType* iconFont, FontTrueType* mimeIconFont,
					  FontTrueType* codIconFont );
};

} // namespace ecode

#endif // ECODE_ICONMANAGER_HPP
