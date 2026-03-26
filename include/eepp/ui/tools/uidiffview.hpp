#ifndef EE_UI_TOOLS_UIDIFFVIEW_HPP
#define EE_UI_TOOLS_UIDIFFVIEW_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiselectbutton.hpp>

namespace EE { namespace UI {

class UIScrollView;

namespace Tools {

class UIDiffEditorPlugin;

class EE_API UIDiffView : public UIWidget {
  public:
	enum class ViewMode { Unified, SideBySide };
	enum class SubLineDiffAlgorithm { LCS, SES };

	static UIDiffView* New();

	static UIScrollView* NewMultiFileDiffViewer( const std::string& patchText );

	static std::vector<std::string> splitDiff( const std::string& multiFileDiff );

	static bool isMultiFileDiff( const std::string& diff );

	virtual ~UIDiffView();

	virtual Uint32 getType() const override;
	virtual bool isType( const Uint32& type ) const override;

	void loadFromPatch( const std::string& patchText, const std::string& originalFilePath = "" );
	void loadFromStrings( const std::string& oldText, const std::string& newText,
						  const std::string& originalFilePath = "" );
	void loadFromFile( const std::string& oldFilePath, const std::string& newFilePath );

	UICodeEditor* getEditor() const { return mEditor; }
	UICodeEditor* getLeftEditor() const { return mLeftEditor; }
	UICodeEditor* getRightEditor() const { return mRightEditor; }

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

  protected:
	UICodeEditor* mEditor{ nullptr };
	UICodeEditor* mLeftEditor{ nullptr };
	UICodeEditor* mRightEditor{ nullptr };
	UISelectButton* mModeToggle{ nullptr };
	UISelectButton* mCompleteViewToggle{ nullptr };
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
	std::shared_ptr<SyntaxDefinition> mSyntaxDef;
	String mFileName;

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
};

} // namespace Tools
}} // namespace EE::UI

#endif // EE_UI_TOOLS_UIDIFFVIEW_HPP
