#ifndef EE_UI_TOOLS_UIIMAGEVIEWER_HPP
#define EE_UI_TOOLS_UIIMAGEVIEWER_HPP

#include <atomic>
#include <memory>

#include <eepp/graphics/image.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

namespace EE::Graphics {

class Texture;

}

namespace EE::System {

class IOStream;

}

namespace EE::UI {

class UIImage;
class UILoader;
class UITextView;

} // namespace EE::UI

namespace EE::UI::Tools {

using namespace EE::Graphics;

/** Image viewer with an optional gallery. Keyboard actions are commands (image-zoom-in,
 * image-zoom-out, image-reset-view, image-flip, image-mirror, image-rotate) bound by
 * getDefaultKeybindings(); applications may rebind them through getKeyBindings(). */
class EE_API UIImageViewer : public UIWidget, public WidgetCommandExecuter {
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

	static KeyBindings::ShortcutMap getDefaultKeybindings();

	virtual ~UIImageViewer();

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	/** Loads the latest request, emitting OnResourceLoaded or OnResourceLoadFailed. reset()
	 * invalidates pending results without waiting for decoding to finish. */
	void loadImageAsync( std::string_view pathOrContents, bool isContents = false,
						 bool loadGallery = true );

	/** Takes ownership of an encoded image buffer without copying it into the worker. */
	void loadImageAsync( std::string&& pathOrContents, bool isContents = false,
						 bool loadGallery = true );

	void loadImageAsync( const char* pathOrContents, bool isContents = false,
						 bool loadGallery = true ) {
		loadImageAsync( std::string_view( pathOrContents ), isContents, loadGallery );
	}

	/** Owns stream until worker decoding finishes. Reads and seeks run on the scene's pool. */
	void loadImageAsync( std::unique_ptr<System::IOStream> stream );

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

	/** Display dimensions after EXIF orientation, without changing stored texture pixels. */
	Sizef getOrientedImageSize() const;

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
	std::atomic<Uint64> mLoadGeneration{ 0 };
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
	ImageOrientation mOrientation{ ImageOrientation::Normal };

	UIImageViewer();

	void loadImage( std::string path, bool isContents, bool loadGallery,
					std::unique_ptr<System::IOStream> stream );

	virtual void onSizeChange() override;

	virtual Uint32 onMessage( const NodeMessage* ) override;

	virtual Uint32 onMouseWheel( const Vector2f& offset, bool flipped ) override;

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;

	void updateImageLayout();

	void updateTextDisplay();

	void zoomStep( Float direction );

	/** Toggle the vertical (flip) or horizontal (mirror) component of the render mode. */
	void toggleRenderMode( bool vertical );
};

} // namespace EE::UI::Tools

#endif
