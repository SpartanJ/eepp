#include <algorithm>
#include <cctype>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/tools/uicolorpicker.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <set>
#include <unordered_map>

using namespace EE::UI::Abstract;
using namespace EE::UI::Models;

namespace EE { namespace UI { namespace Tools {

static const Uint32 FONT_PICKER_STYLE_MARKER = String::hash( "UIFontPickerDialog" );

static const char* FONT_PICKER_STYLE = R"css(
#font_picker {
	margin: 8dp;
	clip: none;
}
#font_picker .column_label {
	margin-bottom: 4dp;
}
#font_picker .footer {
	margin-top: 8dp;
	clip: none;
}
#font_picker .preview_text {
	padding-left: 24dp;
	padding-right: 24dp;
	word-wrap: true;
}
#font_picker .details {
	margin-top: 4dp;
}
#font_picker PushButton.footer_button {
	min-width: 80dp;
	margin-left: 8dp;
}
#font_picker .separator {
	background-color: #FFFFFF44;
}
#font_picker .color_button {
	min-width: 48dp;
	border-color: var(--button-border);
	border-radius: var(--button-radius);
	border-width: var(--border-width);
}
#font_picker .options_panel {
	background-color: var(--list-back);
	border-color: var(--button-border);
	border-radius: var(--button-radius);
	border-width: var(--border-width);
	padding-bottom: 4dp;
	padding-left: 8dp;
	padding-right: 8dp;
	padding-top: 4dp;
}
)css";

static const char* FONT_PICKER_LAYOUT = R"xml(
<LinearLayout id="font_picker" orientation="vertical" layout_width="match_parent" layout_height="match_parent" clip="none">
	<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
		<TextInput id="search_input" layout_width="0dp" layout_weight="1" layout_height="match_parent" />
		<PushButton id="browse_button" layout_width="wrap_content" layout_height="wrap_content" margin-left="8dp" text='@string(font_picker_browse_ellipsis, "Browse...")' />
	</LinearLayout>
	<RelativeLayout layout_width="match_parent" layout_height="0dp" layout_weight="1" margin-top="8dp">
		<LinearLayout id="font_content" orientation="horizontal" layout_width="match_parent" layout_height="match_parent">
			<LinearLayout orientation="vertical" layout_width="0dp" layout_weight="0.28" layout_height="match_parent">
				<TextView class="column_label" layout_width="match_parent" layout_height="wrap_content" text='@string(font_picker_font_family, "Font Family")' />
				<ListView id="family_list" layout_width="match_parent" layout_height="0dp" layout_weight="1" />
			</LinearLayout>
			<LinearLayout orientation="vertical" layout_width="0dp" layout_weight="0.18" layout_height="match_parent" margin-left="8dp">
				<TextView class="column_label" layout_width="match_parent" layout_height="wrap_content" text='@string(font_picker_style, "Style")' />
				<ListView id="style_list" layout_width="match_parent" layout_height="0dp" layout_weight="1" />
			</LinearLayout>
			<LinearLayout id="size_column" orientation="vertical" layout_width="120dp" layout_height="match_parent" margin-left="8dp">
				<TextView class="column_label" layout_width="match_parent" layout_height="wrap_content" text='@string(font_picker_size, "Size")' />
				<ListView id="size_list" layout_width="match_parent" layout_height="0dp" layout_weight="1" />
			</LinearLayout>
			<LinearLayout orientation="vertical" layout_width="0dp" layout_weight="0.54" layout_height="match_parent" margin-left="8dp">
				<TextView class="column_label" layout_width="match_parent" layout_height="wrap_content" text='@string(font_picker_preview, "Preview")' />
				<TextView id="preview_text" class="preview_text" layout_width="match_parent" layout_height="0dp" layout_weight="1" text='The quick brown fox&#10;jumps over the lazy dog' gravity="center" />
				<TextInput id="preview_input" layout_width="match_parent" layout_height="28dp" margin-top="8dp" />
			</LinearLayout>
		</LinearLayout>
		<Loader id="font_loader" lw="64dp" lh="64dp" outline-thickness="6dp" lg="center" visible="false" />
	</RelativeLayout>
	<LinearLayout id="options_row" class="options_panel" orientation="horizontal" layout_width="match_parent" layout_height="wrap_content" margin-top="8dp">
		<CheckBox id="monospace_only" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center" text='@string(font_picker_monospace_only, "Monospace only")' />
		<CheckBox id="antialiasing" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center" margin-left="16dp" text='@string(font_picker_antialiasing, "Antialiasing")' />
		<CheckBox id="underline" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center" margin-left="16dp" text='@string(font_picker_underline, "Underline")' />
		<CheckBox id="strike_through" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center" margin-left="16dp" text='@string(font_picker_strikeout, "Strikeout")' />
		<Widget class="separator" layout_width="1dp" layout_height="match_parent" layout_gravity="center" margin-left="16dp" />
		<TextView id="color_label" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center" margin-left="16dp" text='@string(font_picker_color, "Color:")' gravity="center" />
		<Widget id="color_button" class="color_button" layout_width="48dp" layout_height="20dp" layout_gravity="center" margin-left="8dp" />
	</LinearLayout>
	<TextView id="details_text" class="details" layout_width="match_parent" layout_height="wrap_content" text="" />
	<LinearLayout class="footer" orientation="horizontal" layout_width="match_parent" layout_height="wrap_content" clip="none">
		<Widget layout_width="0dp" layout_weight="1" layout_height="1dp" />
		<PushButton id="cancel_button" class="footer_button" layout_width="80dp" layout_height="wrap_content" text='@string(msg_box_cancel, "Cancel")' />
		<PushButton id="apply_button" class="footer_button" layout_width="80dp" layout_height="wrap_content" text='@string(msg_box_apply, "Apply")' />
		<PushButton id="ok_button" class="footer_button" layout_width="80dp" layout_height="wrap_content" text='@string(msg_box_ok, "Ok")' />
	</LinearLayout>
</LinearLayout>
)xml";

class StyleListModel final : public Model {
  public:
	explicit StyleListModel( const std::vector<UIFontPickerDialog::FontStyleEntry>* data ) :
		mData( data ) {}

	size_t rowCount( const ModelIndex& ) const override { return mData ? mData->size() : 0; }

	size_t columnCount( const ModelIndex& ) const override { return 1; }

	ModelIndex index( int row, int column,
					  const ModelIndex& parent = ModelIndex() ) const override {
		if ( row >= static_cast<int>( rowCount( parent ) ) || column >= 1 )
			return {};
		return Model::index( row, column, parent );
	}

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		if ( role == ModelRole::Display && mData &&
			 index.row() < static_cast<Int64>( mData->size() ) )
			return Variant( ( *mData )[index.row()].label );
		return {};
	}

  private:
	const std::vector<UIFontPickerDialog::FontStyleEntry>* mData{ nullptr };
};

static Uint64 loadFontsTaskTag( const UIFontPickerDialog* dialog ) {
	return reinterpret_cast<Uint64>( dialog );
}

static std::string titleCaseStyleName( std::string label ) {
	std::replace( label.begin(), label.end(), '-', ' ' );
	for ( size_t i = 0; i < label.size(); i++ ) {
		if ( i == 0 || label[i - 1] == ' ' )
			label[i] = static_cast<char>( std::toupper( label[i] ) );
	}
	return label;
}

static std::string stretchLabel( FontStretch stretch ) {
	switch ( stretch ) {
		case FontStretch::UltraCondensed:
			return "Ultra Condensed";
		case FontStretch::ExtraCondensed:
			return "Extra Condensed";
		case FontStretch::Condensed:
			return "Condensed";
		case FontStretch::SemiCondensed:
			return "Semi Condensed";
		case FontStretch::Normal:
			return "";
		case FontStretch::SemiExpanded:
			return "Semi Expanded";
		case FontStretch::Expanded:
			return "Expanded";
		case FontStretch::ExtraExpanded:
			return "Extra Expanded";
		case FontStretch::UltraExpanded:
			return "Ultra Expanded";
	}
	return "";
}

static std::string styleLabel( const FontDesc& desc ) {
	std::string label( stretchLabel( desc.stretch ) );

	if ( desc.weight != FontWeight::Normal ) {
		if ( !label.empty() )
			label += " ";
		label += titleCaseStyleName( Text::fontWeightToString( desc.weight ) );
	}

	if ( desc.italic ) {
		if ( !label.empty() )
			label += " ";
		label += "Italic";
	}

	if ( label.empty() )
		label = "Regular";

	if ( desc.faceIndex != 0 )
		label += " #" + String::toString( desc.faceIndex );
	return label;
}

static Uint32 styleFlags( const UIFontSelection& selection ) {
	Uint32 style = selection.font.italic ? Text::Italic : Text::Regular;
	if ( selection.font.weight >= FontWeight::SemiBold )
		style |= Text::Bold;
	if ( selection.underline )
		style |= Text::Underlined;
	if ( selection.strikeThrough )
		style |= Text::StrikeThrough;
	return style;
}

UIFontPickerDialog* UIFontPickerDialog::New( Uint32 flags ) {
	return eeNew( UIFontPickerDialog, ( flags ) );
}

UIFontPickerDialog::UIFontPickerDialog( Uint32 flags ) : UIWindow(), mFlags( flags ) {
	mVisible = false;
	mStyleConfig.WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL;
	updateWinFlags();

	mSizes = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 32, 48, 64, 72 };
	mSelection.size = 12;

	setTitle( i18n( "font_picker_select_font", "Select Font" ) );

	const Sizef sceneSize( getUISceneNode()->getSize() );
	const Sizef maxSize( eemax( 320.f, sceneSize.getWidth() - PixelDensity::dpToPx( 32 ) ),
						 eemax( 320.f, sceneSize.getHeight() - PixelDensity::dpToPx( 32 ) ) );
	setMinWindowSize(
		Sizef( eemin( 720.f, maxSize.getWidth() ), eemin( 440.f, maxSize.getHeight() ) ) );
	setSizeWithDecoration(
		Sizef( eemin( 940.f, maxSize.getWidth() ), eemin( 620.f, maxSize.getHeight() ) ) );

	loadWidgets();
	if ( getUISceneNode()->getUIThemeManager()->getDefaultTheme() )
		setTheme( getUISceneNode()->getUIThemeManager()->getDefaultTheme() );
	else
		updateDefaultSelectionColor();
	selectInitialRows();
	loadFonts();
}

UIFontPickerDialog::~UIFontPickerDialog() {
	if ( mLoadFontsRequest ) {
		mLoadFontsRequest->alive = false;
		mLoadFontsRequest->dialog.store( nullptr );
	}
	if ( mLoadFontsTaskId && getUISceneNode()->hasThreadPool() )
		getUISceneNode()->getThreadPool()->removeWithTag( loadFontsTaskTag( this ) );
	if ( mColorPicker && mColorPickerCloseCb )
		mColorPicker->getUIWindow()->removeEventListener( mColorPickerCloseCb );
	if ( mColorPicker && !SceneManager::instance()->isShuttingDown() )
		mColorPicker->closePicker();
	mColorPicker = nullptr;
	mColorPickerCloseCb = 0;
	clearBrowseDialog();
}

Uint32 UIFontPickerDialog::getType() const {
	return UI_TYPE_FONTPICKERDIALOG;
}

bool UIFontPickerDialog::isType( const Uint32& type ) const {
	return type == UI_TYPE_FONTPICKERDIALOG ? true : UIWindow::isType( type );
}

void UIFontPickerDialog::setTheme( UITheme* theme ) {
	UIWindow::setTheme( theme );

	if ( mButtonOK ) {
		if ( Drawable* icon =
				 getUISceneNode()->findIconDrawable( "ok", PixelDensity::dpToPxI( 16 ) ) )
			mButtonOK->setIcon( icon );
	}

	if ( mButtonCancel ) {
		if ( Drawable* icon =
				 getUISceneNode()->findIconDrawable( "cancel", PixelDensity::dpToPxI( 16 ) ) )
			mButtonCancel->setIcon( icon );
	}

	if ( mButtonBrowse ) {
		if ( Drawable* icon = getUISceneNode()->findIconDrawable( "document-open",
																  PixelDensity::dpToPxI( 16 ) ) )
			mButtonBrowse->setIcon( icon );
	}

	onThemeLoaded();
	updateDefaultSelectionColor();
}

void UIFontPickerDialog::loadWidgets() {
	UISceneNode* sceneNode = getUISceneNode();
	if ( !sceneNode->getStyleSheet().markerExists( FONT_PICKER_STYLE_MARKER ) ) {
		CSS::StyleSheetParser parser;
		parser.loadFromString( std::string_view{ FONT_PICKER_STYLE } );
		parser.getStyleSheet().setMarker( FONT_PICKER_STYLE_MARKER );
		sceneNode->combineStyleSheet( parser.getStyleSheet() );
	}

	UIWidget* root = sceneNode->loadLayoutFromString( FONT_PICKER_LAYOUT, mContainer );
	root->bind( "font_content", mFontContent );
	root->bind( "font_loader", mFontLoader );
	root->bind( "search_input", mSearchInput );
	root->bind( "family_list", mFamilyList );
	root->bind( "style_list", mStyleList );
	root->bind( "size_list", mSizeList );
	root->bind( "preview_text", mPreviewText );
	root->bind( "preview_input", mPreviewInput );
	root->bind( "details_text", mDetailsText );
	root->bind( "monospace_only", mMonospaceOnly );
	root->bind( "antialiasing", mAntialiasing );
	root->bind( "underline", mUnderline );
	root->bind( "strike_through", mStrikeThrough );
	root->bind( "color_button", mColorButton );
	root->bind( "ok_button", mButtonOK );
	root->bind( "cancel_button", mButtonCancel );
	root->bind( "apply_button", mButtonApply );
	root->bind( "browse_button", mButtonBrowse );

	mSearchInput->setHint( i18n( "font_picker_search_fonts_ellipsis", "Search fonts..." ) );
	mPreviewInput->setText( "The quick brown fox jumps over the lazy dog" );
	mAntialiasing->setChecked( true );
	mMonospaceOnly->setChecked( ( mFlags & MonospaceOnly ) != 0 );
	mMonospaceOnly->setEnabled( ( mFlags & MonospaceOnly ) == 0 );

	if ( ( mFlags & ShowSize ) == 0 ) {
		if ( auto sizeColumn = root->find<UIWidget>( "size_column" ) )
			sizeColumn->setVisible( false )->setEnabled( false );
	}

	if ( ( mFlags & ShowEffects ) == 0 ) {
		mAntialiasing->setVisible( false )->setEnabled( false );
		mUnderline->setVisible( false )->setEnabled( false );
		mStrikeThrough->setVisible( false )->setEnabled( false );
	}

	if ( ( mFlags & ShowColor ) == 0 ) {
		if ( auto colorLabel = root->find<UIWidget>( "color_label" ) )
			colorLabel->setVisible( false )->setEnabled( false );
		mColorButton->setVisible( false )->setEnabled( false );
	}

	if ( ( mFlags & ShowApplyButton ) == 0 )
		mButtonApply->setVisible( false )->setEnabled( false );

	mFamilyList->setHeadersVisible( false );
	mStyleList->setHeadersVisible( false );
	mSizeList->setHeadersVisible( false );
	mFamilyList->setAutoExpandOnSingleColumn( true );
	mStyleList->setAutoExpandOnSingleColumn( true );
	mSizeList->setAutoExpandOnSingleColumn( true );

	mSearchInput->on( Event::OnTextChanged, [this]( const Event* ) { updateFamilies(); } );
	mFamilyList->setOnSelectionChange( [this] {
		if ( !mUpdating ) {
			updateStyles();
			updateSelectionFromLists();
			updatePreview();
		}
	} );
	mStyleList->setOnSelectionChange( [this] {
		if ( !mUpdating ) {
			updateSelectionFromLists();
			updatePreview();
		}
	} );
	mSizeList->setOnSelectionChange( [this] {
		if ( !mUpdating ) {
			updateSelectionFromLists();
			updatePreview();
		}
	} );

	mMonospaceOnly->on( Event::OnValueChange, [this]( const Event* ) { updateFamilies(); } );
	mAntialiasing->on( Event::OnValueChange, [this]( const Event* ) {
		mSelection.antialiasing = mAntialiasing->isChecked();
		updatePreview();
	} );
	mUnderline->on( Event::OnValueChange, [this]( const Event* ) {
		mSelection.underline = mUnderline->isChecked();
		updatePreview();
	} );
	mStrikeThrough->on( Event::OnValueChange, [this]( const Event* ) {
		mSelection.strikeThrough = mStrikeThrough->isChecked();
		updatePreview();
	} );
	mPreviewInput->on( Event::OnTextChanged, [this]( const Event* ) { updatePreview(); } );
	mColorButton->on( Event::MouseClick, [this]( const Event* ) { pickColor(); } );
	mButtonApply->on( Event::MouseClick, [this]( const Event* ) {
		emitPicked();
		sendCommonEvent( Event::OnApply );
	} );
	mButtonBrowse->on( Event::MouseClick, [this]( const Event* ) { browseFont(); } );
}

void UIFontPickerDialog::loadFonts() {
	setFontsLoading( true );

	if ( getUISceneNode()->hasThreadPool() ) {
		auto request = std::make_shared<LoadFontsRequest>();
		request->dialog.store( this );
		mLoadFontsRequest = request;
		UISceneNode* sceneNode = getUISceneNode();
		mLoadFontsTaskId = sceneNode->getThreadPool()->run(
			[request, sceneNode] {
				auto fonts = SystemFontResolver::instance()->enumerate();
				UIFontPickerDialog* dialog = request->dialog.load();
				sceneNode->runOnMainThread(
					[request, fonts = std::move( fonts )]() mutable {
						UIFontPickerDialog* dialog = request->dialog.load();
						if ( !request->alive || dialog == nullptr )
							return;
						dialog->mLoadFontsTaskId = 0;
						dialog->setFonts( std::move( fonts ) );
					},
					Time::Zero, loadFontsTaskTag( dialog ) );
			},
			[]( const Uint64& ) {}, loadFontsTaskTag( this ) );
		return;
	}

	setFonts( SystemFontResolver::instance()->enumerate() );
}

void UIFontPickerDialog::setFonts( std::vector<FontDesc> fonts ) {
	FontDesc selectedFont = mSelection.font;
	mergeFontManagerFonts( fonts );
	for ( const auto& font : mFonts ) {
		if ( std::find_if( fonts.begin(), fonts.end(), [&]( const FontDesc& desc ) {
				 return desc.sameFile( font );
			 } ) == fonts.end() )
			fonts.push_back( font );
	}

	mFonts = std::move( fonts );
	sortFonts();
	updateFontTags();
	setFontsLoading( false );
	updateFamilies();
	if ( !selectedFont.path.empty() || !selectedFont.family.empty() )
		setSelectedFont( selectedFont );
	else
		updatePreview();
}

void UIFontPickerDialog::sortFonts() {
	std::sort( mFonts.begin(), mFonts.end(), []( const auto& lhs, const auto& rhs ) {
		if ( lhs.family != rhs.family )
			return String::toLower( lhs.family ) < String::toLower( rhs.family );
		if ( lhs.weight != rhs.weight )
			return lhs.weight < rhs.weight;
		if ( lhs.italic != rhs.italic )
			return !lhs.italic && rhs.italic;
		if ( lhs.path != rhs.path )
			return lhs.path < rhs.path;
		return lhs.faceIndex < rhs.faceIndex;
	} );
}

void UIFontPickerDialog::mergeFontManagerFonts( std::vector<FontDesc>& fonts ) {
	FontManager::instance()->each( [&]( const auto& res ) {
		if ( res.second == nullptr || res.second->getType() != FontType::TTF )
			return;

		FontDesc desc;
		if ( !static_cast<FontTrueType*>( res.second )->getFontDesc( desc ) )
			return;

		mLoadedFontKeys.insert( desc.getFileKey() );
		if ( std::find_if( fonts.begin(), fonts.end(), [&]( const FontDesc& font ) {
				 return font.sameFile( desc );
			 } ) == fonts.end() )
			fonts.push_back( desc );
	} );
}

void UIFontPickerDialog::updateFontTags() {
	mFontTags.clear();

	std::unordered_map<std::string, Uint32> styleCounts;
	for ( const auto& font : mFonts )
		styleCounts[font.getStyleKey()]++;

	for ( const auto& font : mFonts ) {
		const auto styleCountIt = styleCounts.find( font.getStyleKey() );
		if ( styleCountIt == styleCounts.end() || styleCountIt->second < 2 )
			continue;
		if ( mLoadedFontKeys.find( font.getFileKey() ) == mLoadedFontKeys.end() )
			continue;

		const std::string fileName( FileSystem::fileNameFromPath( font.path ) );
		mFontTags[font.getFileKey()] = fileName.empty() ? font.path : fileName;
	}
}

void UIFontPickerDialog::setFontsLoading( bool loading ) {
	mLoadingFonts = loading;
	if ( mFontContent )
		mFontContent->setVisible( !loading )->setEnabled( !loading );
	if ( mFontLoader )
		mFontLoader->setVisible( loading )->setEnabled( loading );
	if ( mSearchInput )
		mSearchInput->setEnabled( !loading );
	if ( mButtonBrowse )
		mButtonBrowse->setEnabled( !loading );
	if ( mButtonOK )
		mButtonOK->setEnabled( !loading );
	if ( mButtonApply )
		mButtonApply->setEnabled( !loading && ( mFlags & ShowApplyButton ) != 0 );
}

void UIFontPickerDialog::updateDefaultSelectionColor() {
	if ( !mSelectionColorExplicit && mPreviewText )
		mSelection.color = mPreviewText->getFontColor();
}

bool UIFontPickerDialog::wantsMonospaceOnly() const {
	return mMonospaceOnly && mMonospaceOnly->isChecked();
}

void UIFontPickerDialog::updateFamilies() {
	std::string previousFamily = mSelection.font.family;
	if ( !mFamilyList->getSelection().isEmpty() &&
		 mFamilyList->getSelection().first().row() < static_cast<Int64>( mFamilies.size() ) )
		previousFamily = mFamilies[mFamilyList->getSelection().first().row()];

	const std::string query = String::toLower( mSearchInput->getText().toUtf8() );
	std::set<std::string> families;
	for ( const auto& font : mFonts ) {
		if ( wantsMonospaceOnly() && !font.monospace )
			continue;
		if ( !query.empty() && String::toLower( font.family ).find( query ) == std::string::npos )
			continue;
		families.insert( font.family );
	}
	mFamilies.assign( families.begin(), families.end() );
	mFamilyModel = ItemListModel<std::string>::create( mFamilies );

	mUpdating = true;
	mFamilyList->setModel( mFamilyModel );
	mUpdating = false;

	if ( !previousFamily.empty() )
		selectFamily( previousFamily );
	if ( mFamilyList->getSelection().isEmpty() && !mFamilies.empty() )
		mFamilyList->setSelection( mFamilyModel->index( 0 ) );

	updateStyles();
	updateSelectionFromLists();
	updatePreview();
}

void UIFontPickerDialog::updateStyles() {
	mStyles.clear();
	if ( !mFamilyList->getSelection().isEmpty() ) {
		const Int64 row = mFamilyList->getSelection().first().row();
		if ( row >= 0 && row < static_cast<Int64>( mFamilies.size() ) ) {
			const std::string& family = mFamilies[row];
			for ( const auto& font : mFonts ) {
				if ( font.family == family && ( !wantsMonospaceOnly() || font.monospace ) ) {
					std::string label( styleLabel( font ) );
					auto tagIt = mFontTags.find( font.getFileKey() );
					if ( tagIt != mFontTags.end() )
						label += " [" + tagIt->second + "]";
					mStyles.push_back( { label, font } );
				}
			}
		}
	}
	mStyleModel = std::make_shared<StyleListModel>( &mStyles );

	mUpdating = true;
	mStyleList->setModel( mStyleModel );
	mUpdating = false;

	if ( !mStyles.empty() ) {
		const std::string selectedFamily( mStyles.front().desc.family );
		if ( selectedFamily == mSelection.font.family )
			selectStyle( mSelection.font );
		else
			selectRegularStyle();
		if ( mStyleList->getSelection().isEmpty() )
			mStyleList->setSelection( mStyleModel->index( 0 ) );
	}
}

void UIFontPickerDialog::updateSelectionFromLists( bool notify ) {
	UIFontSelection previousSelection = mSelection;
	if ( !mStyleList->getSelection().isEmpty() ) {
		const Int64 row = mStyleList->getSelection().first().row();
		if ( row >= 0 && row < static_cast<Int64>( mStyles.size() ) )
			mSelection.font = mStyles[row].desc;
	}
	if ( !mSizeList->getSelection().isEmpty() ) {
		const Int64 row = mSizeList->getSelection().first().row();
		if ( row >= 0 && row < static_cast<Int64>( mSizes.size() ) )
			mSelection.size = mSizes[row];
	}
	mSelection.antialiasing = mAntialiasing->isChecked();
	mSelection.underline = mUnderline->isChecked();
	mSelection.strikeThrough = mStrikeThrough->isChecked();

	if ( notify && previousSelection != mSelection )
		emitSelectionChanged();
}

void UIFontPickerDialog::emitSelectionChanged() {
	if ( mFontSelectionChangedCb )
		mFontSelectionChangedCb( mSelection );
	sendCommonEvent( Event::OnValueChange );
}

void UIFontPickerDialog::updatePreview() {
	if ( !mPreviewText )
		return;

	FontTrueType* font = FontManager::instance()->getOrLoadSystemFallbackFont( mSelection.font );
	if ( font ) {
		mPreviewText->setFont( font );
		mPreviewInput->setFont( font );
	}

	mPreviewText->setFontSize( PixelDensity::dpToPxI( mSelection.size * 2 ) );
	mPreviewInput->setFontSize( PixelDensity::dpToPxI( 12 ) );
	mPreviewText->setFontStyle( styleFlags( mSelection ) );
	mPreviewInput->setFontStyle( styleFlags( mSelection ) );
	mPreviewText->setFontColor( mSelection.color );
	mPreviewInput->setFontColor( mSelection.color );
	if ( !mPreviewInput->getText().empty() )
		mPreviewText->setText( mPreviewInput->getText() );

	if ( mColorButton )
		mColorButton->setBackgroundColor( mSelection.color );

	if ( mDetailsText ) {
		std::string faceIndexSuffix;
		if ( mSelection.font.faceIndex != 0 )
			faceIndexSuffix = " #" + String::toString( mSelection.font.faceIndex );
		mDetailsText->setText(
			String::format( "%s %s - %u pt - %s%s", mSelection.font.family.c_str(),
							styleLabel( mSelection.font ).c_str(), mSelection.size,
							mSelection.font.path.c_str(), faceIndexSuffix.c_str() ) );
	}
}

void UIFontPickerDialog::selectInitialRows() {
	mSizeModel = ItemListModel<Uint32>::create( mSizes );
	mSizeList->setModel( mSizeModel );
	selectSize( mSelection.size );
	if ( mSizeList->getSelection().isEmpty() )
		mSizeList->setSelection( mSizeModel->index( 4 ) );
}

void UIFontPickerDialog::selectFamily( const std::string& family ) {
	if ( family.empty() || !mFamilyModel )
		return;
	for ( size_t i = 0; i < mFamilies.size(); i++ ) {
		if ( mFamilies[i] == family ) {
			mFamilyList->setSelection( mFamilyModel->index( i ) );
			return;
		}
	}
}

void UIFontPickerDialog::selectRegularStyle() {
	if ( !mStyleModel )
		return;

	for ( size_t i = 0; i < mStyles.size(); i++ ) {
		const auto& style = mStyles[i].desc;
		if ( style.weight == FontWeight::Normal && !style.italic ) {
			mStyleList->setSelection( mStyleModel->index( i ) );
			return;
		}
	}
}

void UIFontPickerDialog::selectStyle( const FontDesc& desc ) {
	if ( desc.family.empty() || !mStyleModel )
		return;
	for ( size_t i = 0; i < mStyles.size(); i++ ) {
		const auto& style = mStyles[i].desc;
		if ( style.sameFile( desc ) && style.sameStyle( desc ) ) {
			mStyleList->setSelection( mStyleModel->index( i ) );
			return;
		}
	}
}

void UIFontPickerDialog::selectSize( Uint32 size ) {
	if ( !mSizeModel )
		return;
	for ( size_t i = 0; i < mSizes.size(); i++ ) {
		if ( mSizes[i] == size ) {
			mSizeList->setSelection( mSizeModel->index( i ) );
			return;
		}
	}
}

void UIFontPickerDialog::emitPicked() {
	updateSelectionFromLists();
	if ( mFontPickedCb )
		mFontPickedCb( mSelection );
}

void UIFontPickerDialog::pickColor() {
	if ( mColorPicker ) {
		mColorPicker->getUIWindow()->toFront();
		return;
	}

	mColorPicker = UIColorPicker::NewWindow( [this]( Color color ) {
		mSelection.color = color;
		mSelectionColorExplicit = true;
		updatePreview();
	} );
	mColorPicker->setColor( mSelection.color );
	mColorPicker->getUIWindow()->center();
	mColorPickerCloseCb =
		mColorPicker->getUIWindow()->on( Event::OnWindowClose, [this]( const Event* ) {
			mColorPicker = nullptr;
			mColorPickerCloseCb = 0;
		} );
}

void UIFontPickerDialog::browseFont() {
	if ( mBrowseDialog ) {
		mBrowseDialog->toFront();
		return;
	}

	mBrowseDialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags, "*.ttf; *.otf; *.woff; *.woff2; *.otb; "
													   "*.bdf; *.ttc" );
	mBrowseDialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	mBrowseDialog->setTitle( i18n( "font_picker_browse_font", "Browse Font" ) );
	mBrowseDialog->setCloseShortcut( KEY_ESCAPE );
	mBrowseDialogOpenCb = mBrowseDialog->on( Event::OpenFile, [this]( const Event* event ) {
		auto* dialog = event->getNode()->asType<UIFileDialog>();
		const std::string path( dialog->getFullPath() );
		if ( addExternalFont( path ) )
			dialog->closeWindow();
	} );
	mBrowseDialogCloseCb = mBrowseDialog->on( Event::OnWindowClose, [this]( const Event* ) {
		mBrowseDialog = nullptr;
		mBrowseDialogOpenCb = 0;
		mBrowseDialogCloseCb = 0;
	} );
	mBrowseDialog->center();
	mBrowseDialog->show();
}

bool UIFontPickerDialog::addExternalFont( const std::string& path, Uint32 faceIndex ) {
	if ( path.empty() )
		return false;

	auto found = std::find_if( mFonts.begin(), mFonts.end(), [&]( const auto& desc ) {
		return desc.sameFile( path, faceIndex );
	} );

	if ( found != mFonts.end() ) {
		if ( !mSearchInput->getText().empty() )
			mSearchInput->setText( "" );
		setSelectedFont( *found );
		return true;
	}

	const std::string fontName(
		FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );
	FontTrueType* font = FontTrueType::New( fontName );
	if ( !font || !font->loadFromFile( path, faceIndex ) ) {
		eeSAFE_DELETE( font );
		return false;
	}

	FontDesc desc;
	if ( !font->getFontDesc( desc ) ) {
		eeSAFE_DELETE( font );
		return false;
	}
	eeSAFE_DELETE( font );

	mLoadedFontKeys.insert( desc.getFileKey() );
	mFonts.push_back( desc );
	sortFonts();
	updateFontTags();

	if ( !mSearchInput->getText().empty() )
		mSearchInput->setText( "" );
	updateFamilies();
	setSelectedFont( desc );
	return true;
}

void UIFontPickerDialog::clearBrowseDialog() {
	if ( !mBrowseDialog )
		return;
	if ( mBrowseDialogOpenCb )
		mBrowseDialog->removeEventListener( mBrowseDialogOpenCb );
	if ( mBrowseDialogCloseCb )
		mBrowseDialog->removeEventListener( mBrowseDialogCloseCb );
	if ( !SceneManager::instance()->isShuttingDown() )
		mBrowseDialog->closeWindow();
	mBrowseDialog = nullptr;
	mBrowseDialogOpenCb = 0;
	mBrowseDialogCloseCb = 0;
}

void UIFontPickerDialog::setSelectedFont( const FontDesc& desc ) {
	UIFontSelection previousSelection = mSelection;
	FontDesc selection = desc;
	if ( !desc.path.empty() ) {
		auto found = std::find_if( mFonts.begin(), mFonts.end(),
								   [&]( const auto& font ) { return font.sameFile( desc ); } );
		if ( found == mFonts.end() && addExternalFont( desc.path, desc.faceIndex ) )
			return;
		if ( found != mFonts.end() )
			selection = *found;
	}

	mSelection.font = selection;
	if ( selection.monospace && mMonospaceOnly )
		mMonospaceOnly->setChecked( true );
	selectFamily( selection.family );
	updateStyles();
	selectStyle( selection );
	updateSelectionFromLists( false );
	if ( previousSelection != mSelection )
		emitSelectionChanged();
	updatePreview();
}

void UIFontPickerDialog::setSelectedFont( const std::string& path, Uint32 faceIndex ) {
	auto found = std::find_if( mFonts.begin(), mFonts.end(), [&]( const auto& desc ) {
		return desc.sameFile( path, faceIndex );
	} );
	if ( found != mFonts.end() ) {
		setSelectedFont( *found );
		return;
	}

	addExternalFont( path, faceIndex );
}

const UIFontSelection& UIFontPickerDialog::getSelection() const {
	return mSelection;
}

void UIFontPickerDialog::setSelection( const UIFontSelection& selection ) {
	mSelection = selection;
	mSelectionColorExplicit = true;
	if ( mAntialiasing )
		mAntialiasing->setChecked( selection.antialiasing );
	if ( mUnderline )
		mUnderline->setChecked( selection.underline );
	if ( mStrikeThrough )
		mStrikeThrough->setChecked( selection.strikeThrough );
	selectSize( selection.size );
	setSelectedFont( selection.font );
}

void UIFontPickerDialog::setOnFontPicked( FontPickedCb cb ) {
	mFontPickedCb = std::move( cb );
}

void UIFontPickerDialog::setOnFontSelectionChanged( FontSelectionChangedCb cb ) {
	mFontSelectionChangedCb = std::move( cb );
}

UIPushButton* UIFontPickerDialog::getButtonOK() const {
	return mButtonOK;
}

UIPushButton* UIFontPickerDialog::getButtonCancel() const {
	return mButtonCancel;
}

UIPushButton* UIFontPickerDialog::getButtonApply() const {
	return mButtonApply;
}

UIPushButton* UIFontPickerDialog::getButtonBrowse() const {
	return mButtonBrowse;
}

UITextInput* UIFontPickerDialog::getSearchInput() const {
	return mSearchInput;
}

UIListView* UIFontPickerDialog::getFamilyList() const {
	return mFamilyList;
}

UIListView* UIFontPickerDialog::getStyleList() const {
	return mStyleList;
}

UIListView* UIFontPickerDialog::getSizeList() const {
	return mSizeList;
}

const KeyBindings::Shortcut& UIFontPickerDialog::getCloseShortcut() const {
	return mCloseShortcut;
}

void UIFontPickerDialog::setCloseShortcut( const KeyBindings::Shortcut& closeShortcut ) {
	mCloseShortcut = closeShortcut;
}

bool UIFontPickerDialog::show() {
	bool ret = UIWindow::show();
	if ( mSearchInput && mSearchInput->isEnabled() )
		mSearchInput->setFocus();
	return ret;
}

void UIFontPickerDialog::onWindowReady() {
	UIWindow::onWindowReady();
	if ( mSearchInput && mSearchInput->isEnabled() )
		mSearchInput->setFocus();
}

Uint32 UIFontPickerDialog::onKeyUp( const KeyEvent& event ) {
	if ( mCloseShortcut && event.getKeyCode() == mCloseShortcut &&
		 ( mCloseShortcut.mod == 0 || ( event.getMod() & mCloseShortcut.mod ) ) ) {
		sendCommonEvent( Event::OnDiscard );
		sendCommonEvent( Event::OnCancel );
		closeWindow();
	}

	return UIWindow::onKeyUp( event );
}

Uint32 UIFontPickerDialog::onMessage( const NodeMessage* msg ) {
	switch ( msg->getMsg() ) {
		case NodeMessage::MouseClick:
			if ( msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( msg->getSender() == mButtonOK ) {
					emitPicked();
					sendCommonEvent( Event::OnConfirm );
					closeWindow();
				} else if ( msg->getSender() == mButtonCancel ) {
					sendCommonEvent( Event::OnDiscard );
					sendCommonEvent( Event::OnCancel );
					closeWindow();
				}
			}
			break;
	}

	return UIWindow::onMessage( msg );
}

}}} // namespace EE::UI::Tools
