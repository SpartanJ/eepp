#ifndef EE_UITEXTUREVIEWER_HPP
#define EE_UITEXTUREVIEWER_HPP

#include <eepp/ui/uirelativelayout.hpp>
#include <set>

using namespace EE::UI;

namespace EE { namespace UI {

class UIGridLayout;;

namespace Tools {

class EE_API UITextureViewer : public UIRelativeLayout {
  public:
	static UITextureViewer* New();

	~UITextureViewer();

  protected:
	UIGridLayout* mGridLayout{ nullptr };
	UIRelativeLayout* mImageLayout{ nullptr };
	UnorderedMap<Texture*, Uint32> mCbs;
	Uint32 mLoaderCb{ 0 };

	UITextureViewer();

	void init();

	void insertTexture( Texture* tex );

	void setImage( Drawable* );
};

} // namespace Tools
}} // namespace EE::UI

#endif // EE_UITEXTUREVIEWER_HPP
