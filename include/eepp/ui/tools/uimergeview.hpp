#ifndef EE_UI_TOOLS_UIMERGEVIEW_HPP
#define EE_UI_TOOLS_UIMERGEVIEW_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uistacklayout.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

namespace EE::UI::Tools {

class UIMergeEditorPlugin;

struct EE_API MergeVersion {
	String text;
	String label;
	std::string objectId;
	Uint32 mode{ 0 };
	bool present{ false };
};

struct EE_API MergeInput {
	MergeVersion base;
	MergeVersion stage2;
	MergeVersion stage3;
	std::shared_ptr<Doc::TextDocument> resultDocument;
	std::string path;
	String resultLabel;
	String missingVersionLabel;
};

class EE_API UIMergeView : public UILinearLayout, public WidgetCommandExecuter {
  public:
	enum class Order { Stage2ThenStage3, Stage3ThenStage2 };

	struct ConflictBlock {
		TextRange range;
		String stage2;
		String stage3;
	};

	static UIMergeView* New();
	static std::vector<ConflictBlock> parseConflictBlocks( const String& text );

	UIMergeView();
	virtual ~UIMergeView();
	virtual Uint32 getType() const override;
	virtual bool isType( const Uint32& type ) const override;

	void load( MergeInput input );
	UICodeEditor* getLeftEditor() const { return mLeftEditor; }
	UICodeEditor* getResultEditor() const { return mResultEditor; }
	UICodeEditor* getRightEditor() const { return mRightEditor; }
	const MergeInput& getInput() const { return mInput; }

	bool hasUnresolvedMarkerBlocks() const;
	void recreateConflict();
	void setRecreateConflictCallback( CommandCallback callback ) {
		mRecreateConflictCallback = std::move( callback );
	}
	void goToNextConflict();
	void goToPreviousConflict();
	void acceptStage2( size_t block );
	void acceptStage3( size_t block );
	void acceptBoth( size_t block, Order order = Order::Stage2ThenStage3 );

	void setSyntaxColorScheme( const SyntaxColorScheme& colorScheme );
	void setToolbarVisible( bool visible );
	bool isToolbarVisible() const { return mToolbarVisible; }
	UIPushButton* addToolbarAction( const std::string& command, const String& text,
									const KeyBindings::Shortcut& shortcut, const std::string& icon,
									CommandCallback callback );

  protected:
	MergeInput mInput;
	UICodeEditor* mLeftEditor{ nullptr };
	UICodeEditor* mResultEditor{ nullptr };
	UICodeEditor* mRightEditor{ nullptr };
	UIStackLayout* mToolbar{ nullptr };
	UILinearLayout* mEditorsLayout{ nullptr };
	size_t mCurrentBlock{ 0 };
	bool mSyncingScroll{ false };
	bool mApplyingBlock{ false };
	bool mToolbarVisible{ true };

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;
	void replaceBlock( size_t block, const String& replacement );
	void refreshBlocks();
	std::vector<ConflictBlock> mBlocks;
	String mOriginalResultText;
	CommandCallback mRecreateConflictCallback;
	std::unique_ptr<UIMergeEditorPlugin> mLeftPlugin;
	std::unique_ptr<UIMergeEditorPlugin> mResultPlugin;
	std::unique_ptr<UIMergeEditorPlugin> mRightPlugin;

	void refreshHighlights();
	void syncScroll( UICodeEditor* source );
	void createToolbar();
	void goToConflict( size_t block );
	size_t getCurrentConflictBlock();
	void acceptCurrentStage2();
	void acceptCurrentStage3();
	void acceptCurrentBoth();
	bool executeMergeKeyBinding( const KeyEvent& event );
	friend class UIMergeEditorPlugin;
};

} // namespace EE::UI::Tools

#endif
