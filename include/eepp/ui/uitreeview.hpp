#ifndef EE_UI_UITREEVIEW_HPP
#define EE_UI_UITREEVIEW_HPP

#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uitablerow.hpp>
#include <memory>
#include <unordered_map>

using namespace EE::UI::Abstract;

namespace EE { namespace UI {

class UITableRow;

class EE_API UITreeViewCell : public UITableCell {
  public:
	static UITreeViewCell* New() { return eeNew( UITreeViewCell, () ); }

	Uint32 getType() const { return UI_TYPE_TREEVIEW_CELL; }

	bool isType( const Uint32& type ) const {
		return UITreeViewCell::getType() == type ? true : UITableCell::isType( type );
	}

	UIImage* getImage() const { return mImage; }

	Rectf calculatePadding() const {
		Sizef size;
		Rectf autoPadding;
		if ( mFlags & UI_AUTO_PADDING ) {
			autoPadding = makePadding( true, true, true, true );
			if ( autoPadding != Rectf() )
				autoPadding = PixelDensity::dpToPx( autoPadding );
		}
		if ( mPaddingPx.Top > autoPadding.Top )
			autoPadding.Top = mPaddingPx.Top;
		if ( mPaddingPx.Bottom > autoPadding.Bottom )
			autoPadding.Bottom = mPaddingPx.Bottom;
		if ( mPaddingPx.Left > autoPadding.Left )
			autoPadding.Left = mPaddingPx.Left;
		if ( mPaddingPx.Right > autoPadding.Right )
			autoPadding.Right = mPaddingPx.Right;
		autoPadding.Left += mIndent;
		return autoPadding;
	}

	void setIndentation( const Float& indent ) {
		if ( mIndent != indent ) {
			mIndent = indent;
			updateLayout();
		}
	}

	const Float& getIndentation() const { return mIndent; }

  protected:
	mutable UIImage* mImage{ nullptr };
	Float mIndent{ 0 };

	UITreeViewCell( const std::function<UITextView*( UIPushButton* )>& newTextViewCb = nullptr ) :
		UITableCell( "treeview::cell", newTextViewCb ) {
		mTextBox->setElementTag( mTag + "::text" );
		mIcon->setElementTag( mTag + "::icon" );
		mInnerWidgetOrientation = InnerWidgetOrientation::Left;
		auto cb = [&]( const Event* ) { updateLayout(); };
		mImage = UIImage::NewWithTag( mTag + "::expander" );
		mImage->setScaleType( UIScaleType::FitInside )
			->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
			->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER )
			->setParent( const_cast<UITreeViewCell*>( this ) )
			->setVisible( false )
			->setEnabled( false );
		mImage->addEventListener( Event::OnPaddingChange, cb );
		mImage->addEventListener( Event::OnMarginChange, cb );
		mImage->addEventListener( Event::OnSizeChange, cb );
		mImage->addEventListener( Event::OnVisibleChange, cb );
	}

	virtual UIWidget* getExtraInnerWidget() const { return mImage; }
};

class EE_API UITreeView : public UIAbstractTableView {
  public:
	static UITreeView* New();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	const Float& getIndentWidth() const;

	void setIndentWidth( const Float& indentWidth );

	virtual Sizef getContentSize() const;

	virtual void drawChilds();

	virtual Node* overFind( const Vector2f& point );

	bool isExpanded( const ModelIndex& index ) const;

	void expandAll( const ModelIndex& index = {} );

	void collapseAll( const ModelIndex& index = {} );

	UIIcon* getExpandIcon() const;

	void setExpandedIcon( EE::UI::UIIcon* expandIcon );

	void setExpandedIcon( const std::string& expandIcon );

	UIIcon* getContractIcon() const;

	void setContractedIcon( EE::UI::UIIcon* contractIcon );

	void setContractedIcon( const std::string& contractIcon );

	bool getExpandersAsIcons() const;

	void setExpandersAsIcons( bool expandersAsIcons );

	Float getMaxColumnContentWidth( const size_t& colIndex, bool bestGuess = false );

	const size_t& getExpanderIconSize() const;

	void setExpanderIconSize( const size_t& expanderSize );

	virtual ModelIndex findRowWithText( const std::string& text, const bool& caseSensitive = false,
										const bool& exactMatch = false ) const;

	virtual ModelIndex selectRowWithPath( std::string path );

	virtual void setSelection( const ModelIndex& index, bool scrollToSelection = true,
							   bool openModelIndexTree = true );

	virtual void openModelIndexParentTree( const ModelIndex& index );

	bool getFocusOnSelection() const;

	void setFocusOnSelection( bool focusOnSelection );

	bool tryOpenModelIndex( const ModelIndex& index, bool forceUpdate = true );

	void updateContentSize();

  protected:
	enum class IterationDecision {
		Continue,
		Break,
		Stop,
	};

	Float mIndentWidth;
	Sizef mContentSize;
	UIIcon* mExpandIcon{ nullptr };
	UIIcon* mContractIcon{ nullptr };
	size_t mExpanderIconSize{ 16 };
	bool mExpandersAsIcons{ false };
	bool mFocusOnSelection{ true };
	bool mFocusSelectionDirty{ false };

	UITreeView();

	virtual void createOrUpdateColumns( bool resetColumnData );

	struct MetadataForIndex {
		bool open{ false };
	};

	typedef std::function<IterationDecision( const int&, const ModelIndex&, const size_t&,
											 const Float& )>
		TreeViewCallback;

	void traverseTree( TreeViewCallback ) const;

	mutable std::map<void*, MetadataForIndex> mViewMetadata;

	virtual size_t getItemCount() const;

	UITreeView::MetadataForIndex& getIndexMetadata( const ModelIndex& index ) const;

	virtual void onColumnSizeChange( const size_t& colIndex, bool fromUserInteraction = false );

	virtual UIWidget* updateCell( const int& rowIndex, const ModelIndex& index,
								  const size_t& indentLevel, const Float& yOffset );

	virtual UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual void onOpenTreeModelIndex( const ModelIndex& index, bool open );

	virtual void onSortColumn( const size_t& colIndex );

	void setAllExpanded( const ModelIndex& index = {}, bool expanded = true );

	virtual UIWidget* setupCell( UITableCell* widget, UIWidget* rowWidget,
								 const ModelIndex& index );

	virtual void onModelSelectionChange();

	virtual void bindNavigationClick( UIWidget* widget );
};

}} // namespace EE::UI

#endif // EE_UI_UITREEVIEW_HPP
