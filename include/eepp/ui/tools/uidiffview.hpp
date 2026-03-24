#ifndef EE_UI_TOOLS_UIDIFFVIEW_HPP
#define EE_UI_TOOLS_UIDIFFVIEW_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiselectbutton.hpp>

namespace EE { namespace UI { namespace Tools {

class UIDiffEditorPlugin;

class EE_API UIDiffView : public UIWidget {
  public:
	enum class ViewMode { Unified, SideBySide };
	enum class SubLineDiffAlgorithm { LCS, SES };

	static UIDiffView* New();

	virtual ~UIDiffView();

	virtual Uint32 getType() const override;
	virtual bool isType( const Uint32& type ) const override;

	void loadFromPatch( const std::string& patchText, const std::string& originalFilePath = "" );
	void loadFromStrings( const std::string& oldText, const std::string& newText );
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
	std::shared_ptr<SyntaxDefinition> mSyntaxDef;

	UIDiffView();

	virtual void onSizeChange() override;

	void createEditor( UICodeEditor*& editor, std::unique_ptr<UIDiffEditorPlugin>& plugin );
	void syncScroll( UICodeEditor* source, UICodeEditor* target, bool emitEvent = false );
	void updateModeButton();
	void computeSubLineDiff( DiffLine& oldLine, DiffLine& newLine );
	void updateEditorsText();
	void updateButtonsText();
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UIDIFFVIEW_HPP
