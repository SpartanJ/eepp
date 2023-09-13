#ifndef EE_UITEXTUREVIEWER_HPP
#define EE_UITEXTUREVIEWER_HPP

#include <eepp/ui/uilinearlayout.hpp>
#include <set>

using namespace EE::UI;

namespace EE { namespace UI {

class UIGridLayout;

namespace Tools {

class EE_API UITextureViewer : public UILinearLayout {
  public:
	static UITextureViewer* New();

	~UITextureViewer();

  protected:
	UIGridLayout* mGridLayout;
	UnorderedMap<Texture*, Uint32> mCbs;
	Uint32 mLoaderCb;

	UITextureViewer();

	void init();

	void insertTexture( Texture* tex );
};

} // namespace Tools
}} // namespace EE::UI

#endif // EE_UITEXTUREVIEWER_HPP
