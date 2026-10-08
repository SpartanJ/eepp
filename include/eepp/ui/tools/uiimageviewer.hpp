#ifndef EE_UI_TOOLS_UIIMAGEVIEWER_HPP
#define EE_UI_TOOLS_UIIMAGEVIEWER_HPP

#include <atomic>

#include <eepp/ui/uiwidget.hpp>

namespace EE::Graphics {
class Texture;
}

namespace EE::UI {
class UIImage;
class UILoader;
class UITextView;
} // namespace EE::UI

namespace EE::UI::Tools {

class EE_API UIImageViewer : public UIWidget {
  public:
	enum DisplayOptions {
		DisplayName = 1 << 0,
		DisplayPath = 1 << 1,
		DisplayDimensions = 1 << 2,
		DisplayGalleryPosition = 1 << 3,
		DisplaySize = 1 << 4,
		DisplayType = 1 << 5,
	};

	static Uint32 displayOptionsFromString( std::string_view );

	static std::string displayOptionsToString( Uint32 opt );

	static UIImageViewer* New();

	virtual ~UIImageViewer();

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	void loadImageAsync( std::string_view pathOrContents, bool isContents = false,
						 bool loadGallery = true );

	UILoader* getLoader() const { return mLoader; }

	UIImage* getImage() const { return mImage; }

	void reset();

	Float getMinScale() { return mMinScale; }

	Float getMaxScale() { return mMaxScale; }

	void setMinScale( Float scale ) { mMinScale = scale; }

	void setMaxScale( Float scale ) { mMaxScale = scale; }

	const std::string& getGalleryPath() const;

	const std::string& getImageName() const;

	std::string getImagePath() const;

	void setDisplayOptions( Uint32 opt );

	Uint32 getDisplayOptions() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute ) override;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const override;

	virtual std::vector<PropertyId> getPropertiesImplemented() const override;

	void resetImageView();

	/** Recalculates the displayed image size and position when the viewer size changes. */
	void setAutoFitOnSizeChange( bool enabled ) { mAutoFitOnSizeChange = enabled; }

	bool getAutoFitOnSizeChange() const { return mAutoFitOnSizeChange; }

	/**
	 * Uses the image's native pixel dimensions when it fits in the viewer, independently of the
	 * UI pixel density. Images larger than the viewer are still scaled down to fit.
	 */
	void setUseNativeImageSize( bool enabled ) { mUseNativeImageSize = enabled; }

	bool getUseNativeImageSize() const { return mUseNativeImageSize; }

	void setPreserveImageView( bool preserve ) { mPreserveImageView = preserve; }

	bool getPreserveImageView() const { return mPreserveImageView; }

	bool isLoading() const { return mLoading; }

	bool hasImage() const;

  protected:
	UILoader* mLoader{ nullptr };
	UIImage* mImage{ nullptr };
	UITextView* mTextView{ nullptr };
	Vector2f mMouseMiddleStartClick;
	Float mMinScale{ 0.25f };
	Float mMaxScale{ 5.f };
	std::string mGalleryPath;
	std::vector<std::string> mGalleryFiles;
	Int64 mGalleryImageIndex{ 0 };
	std::atomic<int> mLoading{ 0 };
	bool mMouseMiddlePressing{ false };
	std::atomic<bool> mGalleryLoaderShouldAbort{ false };
	std::atomic<bool> mClosing{ false };
	bool mHasGallery{ false };
	Float mInitialScale;
	Mutex mGalleryMutex;
	Int64 mCurFileSize{ 0 };
	Image::Format mCurFileType;
	Uint32 mDisplayOptions{ 0 };
	bool mPreserveImageView{ false };
	bool mAutoFitOnSizeChange{ false };
	bool mUseNativeImageSize{ false };

	UIImageViewer();

	virtual void onSizeChange() override;

	virtual Uint32 onMessage( const NodeMessage* ) override;

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;

	void updateImageLayout();

	void updateTextDisplay();
};

} // namespace EE::UI::Tools

#endif
