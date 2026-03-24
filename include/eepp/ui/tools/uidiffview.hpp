#ifndef EE_UI_TOOLS_UIDIFFVIEW_HPP
#define EE_UI_TOOLS_UIDIFFVIEW_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI { namespace Tools {

class UIDiffEditorPlugin;

class EE_API UIDiffView : public UIWidget {
  public:
	enum class ViewMode { Unified, SideBySide };

	static UIDiffView* New();

	virtual ~UIDiffView();

	virtual Uint32 getType() const override;
	virtual bool isType( const Uint32& type ) const override;

	void loadFromPatch( const std::string& patchText );
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

	void setViewMode( ViewMode mode );
	ViewMode getViewMode() const { return mViewMode; }

	void setViewModeToggleVisible( bool visible );
	bool isViewModeToggleVisible() const { return mViewModeToggleVisible; }

  protected:
	UICodeEditor* mEditor{ nullptr };
	UICodeEditor* mLeftEditor{ nullptr };
	UICodeEditor* mRightEditor{ nullptr };
	UIPushButton* mModeToggle{ nullptr };
	std::unique_ptr<UIDiffEditorPlugin> mPlugin;
	std::unique_ptr<UIDiffEditorPlugin> mLeftPlugin;
	std::unique_ptr<UIDiffEditorPlugin> mRightPlugin;
	std::vector<DiffLine> mLines;
	ViewMode mViewMode{ ViewMode::Unified };
	bool mViewModeToggleVisible{ true };

	UIDiffView();

	virtual void onSizeChange() override;

	void createEditor( UICodeEditor*& editor, std::unique_ptr<UIDiffEditorPlugin>& plugin );
	void syncScroll( UICodeEditor* source, UICodeEditor* target, bool emitEvent = false );
	void updateModeButton();
	void computeSubLineDiff( DiffLine& oldLine, DiffLine& newLine );
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UIDIFFVIEW_HPP
