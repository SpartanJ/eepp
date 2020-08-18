#ifndef EE_UI_UITABLEHEADERCOLUMN_HPP
#define EE_UI_UITABLEHEADERCOLUMN_HPP

#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

namespace Abstract {
class UIAbstractTableView;
}
using namespace Abstract;

class EE_API UITableHeaderColumn : public UIPushButton {
  public:
	UITableHeaderColumn( const std::string& parentTag, UIAbstractTableView* view,
						 const size_t& colIndex );

	virtual UIWidget* getExtraInnerWidget() const;

  protected:
	UIAbstractTableView* mView;
	size_t mColIndex;
	mutable UIImage* mImage{nullptr};

	Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags );

	Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	Uint32 onDrag( const Vector2f& position, const Uint32&, const Sizef& dragDiff );

	Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	Uint32 onDragStop( const Vector2i& pos, const Uint32& flags );

	Sizef updateLayout();

	virtual void updateSortIconPosition();
};

}} // namespace EE::UI

#endif // EE_UI_UITABLEHEADERCOLUMN_HPP
