#ifndef EE_UI_TOOLS_UIFONTPICKERDIALOG_HPP
#define EE_UI_TOOLS_UIFONTPICKERDIALOG_HPP

#include <atomic>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/system/color.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace EE { namespace UI {
class UICheckBox;
class UIFileDialog;
class UILoader;
class UIListView;
class UIPushButton;
class UITextInput;
class UITextView;
}} // namespace EE::UI

namespace EE { namespace UI { namespace Tools {

class UIColorPicker;

struct UIFontSelection {
	FontDesc font;
	Uint32 size{ 12 };
	bool underline{ false };
	bool strikeThrough{ false };
	bool antialiasing{ true };
	Color color{ Color::White };

	bool operator==( const UIFontSelection& other ) const {
		return font == other.font && size == other.size && underline == other.underline &&
			   strikeThrough == other.strikeThrough && antialiasing == other.antialiasing &&
			   color == other.color;
	}

	bool operator!=( const UIFontSelection& other ) const { return !( *this == other ); }
};

class EE_API UIFontPickerDialog : public UIWindow {
  public:
	using FontPickedCb = std::function<void( const UIFontSelection& )>;
	using FontSelectionChangedCb = std::function<void( const UIFontSelection& )>;

	enum Flags {
		MonospaceOnly = 1 << 0,
		ShowSize = 1 << 1,
		ShowEffects = 1 << 2,
		ShowColor = 1 << 3,
		ShowApplyButton = 1 << 4,
		DefaultFlags = ShowSize | ShowEffects | ShowColor,
	};

	struct FontStyleEntry {
		std::string label;
		FontDesc desc;
	};

	static UIFontPickerDialog* New( Uint32 flags = DefaultFlags );

	virtual ~UIFontPickerDialog();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* theme );

	void setSelectedFont( const FontDesc& desc );

	void setSelectedFont( const std::string& path, Uint32 faceIndex = 0 );

	const UIFontSelection& getSelection() const;

	void setSelection( const UIFontSelection& selection );

	void setOnFontPicked( FontPickedCb cb );

	void setOnFontSelectionChanged( FontSelectionChangedCb cb );

	UIPushButton* getButtonOK() const;

	UIPushButton* getButtonCancel() const;

	UIPushButton* getButtonApply() const;

	UIPushButton* getButtonBrowse() const;

	UITextInput* getSearchInput() const;

	UIListView* getFamilyList() const;

	UIListView* getStyleList() const;

	UIListView* getSizeList() const;

	const KeyBindings::Shortcut& getCloseShortcut() const;

	void setCloseShortcut( const KeyBindings::Shortcut& closeShortcut );

	virtual bool show();

  protected:
	Uint32 mFlags{ DefaultFlags };
	KeyBindings::Shortcut mCloseShortcut{ KEY_UNKNOWN };
	UIFontSelection mSelection;
	FontPickedCb mFontPickedCb;
	FontSelectionChangedCb mFontSelectionChangedCb;
	std::vector<FontDesc> mFonts;
	std::vector<std::string> mFamilies;
	std::vector<FontStyleEntry> mStyles;
	std::vector<Uint32> mSizes;
	UnorderedSet<std::string> mLoadedFontKeys;
	UnorderedMap<std::string, std::string> mFontTags;
	std::shared_ptr<Models::Model> mFamilyModel;
	std::shared_ptr<Models::Model> mStyleModel;
	std::shared_ptr<Models::Model> mSizeModel;
	UIWidget* mFontContent{ nullptr };
	UILoader* mFontLoader{ nullptr };
	UITextInput* mSearchInput{ nullptr };
	UIListView* mFamilyList{ nullptr };
	UIListView* mStyleList{ nullptr };
	UIListView* mSizeList{ nullptr };
	UITextView* mPreviewText{ nullptr };
	UITextInput* mPreviewInput{ nullptr };
	UITextView* mDetailsText{ nullptr };
	UIColorPicker* mColorPicker{ nullptr };
	UIFileDialog* mBrowseDialog{ nullptr };
	Uint32 mColorPickerCloseCb{ 0 };
	Uint32 mBrowseDialogCloseCb{ 0 };
	Uint32 mBrowseDialogOpenCb{ 0 };
	UICheckBox* mMonospaceOnly{ nullptr };
	UICheckBox* mAntialiasing{ nullptr };
	UICheckBox* mUnderline{ nullptr };
	UICheckBox* mStrikeThrough{ nullptr };
	UIWidget* mColorButton{ nullptr };
	UIPushButton* mButtonOK{ nullptr };
	UIPushButton* mButtonCancel{ nullptr };
	UIPushButton* mButtonApply{ nullptr };
	UIPushButton* mButtonBrowse{ nullptr };
	bool mUpdating{ false };
	bool mLoadingFonts{ false };
	bool mSelectionColorExplicit{ false };
	Uint64 mLoadFontsTaskId{ 0 };

	struct LoadFontsRequest {
		std::atomic<bool> alive{ true };
		std::atomic<UIFontPickerDialog*> dialog{ nullptr };
	};
	std::shared_ptr<LoadFontsRequest> mLoadFontsRequest;

	UIFontPickerDialog( Uint32 flags = DefaultFlags );

	virtual void onWindowReady();

	virtual Uint32 onKeyUp( const KeyEvent& event );

	virtual Uint32 onMessage( const NodeMessage* msg );

	void loadWidgets();

	void loadFonts();

	void setFonts( std::vector<FontDesc> fonts );

	void sortFonts();

	void mergeFontManagerFonts( std::vector<FontDesc>& fonts );

	void updateFontTags();

	void setFontsLoading( bool loading );

	void updateDefaultSelectionColor();

	void updateFamilies();

	void updateStyles();

	void updateSelectionFromLists( bool notify = true );

	void emitSelectionChanged();

	void updatePreview();

	void selectInitialRows();

	void selectFamily( const std::string& family );

	void selectRegularStyle();

	void selectStyle( const FontDesc& desc );

	void selectSize( Uint32 size );

	void emitPicked();

	void pickColor();

	void browseFont();

	bool addExternalFont( const std::string& path, Uint32 faceIndex = 0 );

	void clearBrowseDialog();

	bool wantsMonospaceOnly() const;
};

}}} // namespace EE::UI::Tools

#endif
