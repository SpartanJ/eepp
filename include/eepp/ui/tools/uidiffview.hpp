#ifndef EE_UI_TOOLS_UIDIFFVIEW_HPP
#define EE_UI_TOOLS_UIDIFFVIEW_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

namespace EE {

namespace Graphics {
class Sprite;
}

namespace UI {

class UIScrollView;

namespace Tools {

class UIImageViewer;
class UIDiffEditorPlugin;

class EE_API UIDiffView : public UIWidget, public WidgetCommandExecuter {
  public:
	enum class ViewMode { Unified, SideBySide };
	enum class SubLineDiffAlgorithm { LCS, SES };

	static UIDiffView* New();

	static UIScrollView* NewMultiFileDiffViewer( const std::string& patchText,
												 const std::string& repoPath = "" );

	static std::vector<std::string> splitDiff( const std::string& multiFileDiff );

	static bool isMultiFileDiff( const std::string& diff );

	virtual ~UIDiffView();

	virtual Uint32 getType() const override;
	virtual bool isType( const Uint32& type ) const override;

	void loadFromPatch( const std::string& patchText, const std::string& originalFilePath = "",
						const std::string& oldFilePath = "", const std::string& repoPath = "" );
	void loadFromStrings( const std::string& oldText, const std::string& newText,
						  const std::string& originalFilePath = "" );
	void loadFromFile( const std::string& oldFilePath, const std::string& newFilePath );

	UICodeEditor* getEditor() const { return mEditor; }
	UICodeEditor* getLeftEditor() const { return mLeftEditor; }
	UICodeEditor* getRightEditor() const { return mRightEditor; }
	UIImageViewer* getLeftImageViewer() const { return mLeftImageViewer; }
	UIImageViewer* getRightImageViewer() const { return mRightImageViewer; }

	enum class DiffLineType { Common, Added, Removed, Header };
	struct DiffLine {
		DiffLineType type{ DiffLineType::Common };
		String text;
		Int64 oldLineNum{ 0 };
		Int64 newLineNum{ 0 };
		std::vector<TextRange> subLineChanges;
	};

	const std::vector<DiffLine>& getDiffLines() const { return mLines; }
	const std::vector<size_t>& getViewLines() const { return mViewLines; }

	void setViewMode( ViewMode mode );
	ViewMode getViewMode() const { return mViewMode; }

	void setViewModeToggleVisible( bool visible );
	bool isViewModeToggleVisible() const { return mViewModeToggleVisible; }

	void setCompleteView( bool complete );
	bool isCompleteView() const { return mShowCompleteView; }

	void setCompleteViewToggleVisible( bool visible );
	bool isCompleteViewToggleVisible() const { return mCompleteViewToggleVisible; }

	void setSubLineDiffAlgorithm( SubLineDiffAlgorithm algo );
	SubLineDiffAlgorithm getSubLineDiffAlgorithm() const { return mSubLineDiffAlgorithm; }

	void setSyntaxColorScheme( const SyntaxColorScheme& colorScheme );

	void setHeadersVisible( bool visible );

	bool areHeadersVisible() const { return mHeadersVisible; }

	const String& getFileName() const { return mFileName; }

	bool isImageDiff() const { return mIsImageDiff; }

	void setAutoDeleteOldTempImage( bool set ) { mAutoDeleteOldTempImage = set; }

  protected:
	UICodeEditor* mEditor{ nullptr };
	UICodeEditor* mLeftEditor{ nullptr };
	UICodeEditor* mRightEditor{ nullptr };
	UIImageViewer* mLeftImageViewer{ nullptr };
	UIImageViewer* mRightImageViewer{ nullptr };
	UIImageViewer* mDiffImageViewer{ nullptr };
	UISelectButton* mModeToggle{ nullptr };
	UISelectButton* mCompleteViewToggle{ nullptr };
	Sprite* mSprite{ nullptr };
	std::unique_ptr<UIDiffEditorPlugin> mPlugin;
	std::unique_ptr<UIDiffEditorPlugin> mLeftPlugin;
	std::unique_ptr<UIDiffEditorPlugin> mRightPlugin;
	std::vector<DiffLine> mLines;
	std::vector<size_t> mViewLines;
	ViewMode mViewMode{ ViewMode::Unified };
	SubLineDiffAlgorithm mSubLineDiffAlgorithm{ SubLineDiffAlgorithm::LCS };
	bool mViewModeToggleVisible{ true };
	bool mShowCompleteView{ false };
	bool mCompleteViewToggleVisible{ true };
	bool mHeadersVisible{ false };
	bool mIsImageDiff{ false };
	bool mAutoDeleteOldTempImage{ false };
	std::shared_ptr<SyntaxDefinition> mSyntaxDef;
	String mFileName;
	std::string mImageDiffOldPath;
	std::string mImageDiffNewPath;

	UIDiffView();

	virtual void onSizePolicyChange() override;

	virtual void onAutoSize() override;

	virtual void onSizeChange() override;

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;

	void createEditor( UICodeEditor*& editor, std::unique_ptr<UIDiffEditorPlugin>& plugin );
	void syncScroll( UICodeEditor* source, UICodeEditor* target, bool emitEvent = false );
	void updateModeButton();
	void computeSubLineDiff( DiffLine& oldLine, DiffLine& newLine );
	void updateEditorsText();
	void updateButtonsText();
	void createImageViewers();
	bool loadImageDiffFromPaths( const std::string& oldFilePath, const std::string& newFilePath );
	void updateImageDiffView();
	void resetToTextDiffView();
	void imageDisplayState( bool& displayDiffImage, bool& displayLeftImage );
	void updateImagesPosAndSize();
};

} // namespace Tools
} // namespace UI
} // namespace EE

#endif // EE_UI_TOOLS_UIDIFFVIEW_HPP
