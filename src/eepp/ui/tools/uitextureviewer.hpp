#ifndef EE_UITEXTUREVIEWER_HPP
#define EE_UITEXTUREVIEWER_HPP

#include <eepp/graphics/texturefactory.hpp>
#include <eepp/ui/uirelativelayout.hpp>

using namespace EE::UI;

namespace EE { namespace UI {

class UIGridLayout;

namespace Tools {

class EE_API UITextureViewer : public UIRelativeLayout {
  public:
	static UITextureViewer* New();

	virtual ~UITextureViewer();

	virtual void scheduledUpdate( const Time& time );

  protected:
	struct TextureEntry {
		TextureWeakPtr texture;
		UIWidget* preview{ nullptr };
		bool seen{ false };
	};

	UIGridLayout* mGridLayout{ nullptr };
	UIRelativeLayout* mImageLayout{ nullptr };
	UnorderedMap<Uint64, TextureEntry> mTextures;
	TexturePtr mSelectedTexture;
	Uint64 mLastRegistryGeneration{ 0 };

	UITextureViewer();

	void init();

	void refreshTextures();

	void insertTexture( const TextureRegistryRecord& record );

	void setImage( TexturePtr texture );

	void clearSelectedTexture();
};

} // namespace Tools

}} // namespace EE::UI

#endif // EE_UITEXTUREVIEWER_HPP
