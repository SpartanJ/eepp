#ifndef EE_UI_UIGRIDLAYOUT
#define EE_UI_UIGRIDLAYOUT

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIGridLayout : public UILayout {
	public:
		enum ElementMode
		{
			Size,
			Weight
		};

		static UIGridLayout * New();
		
		UIGridLayout();
		
		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		Sizei getSpan() const;

		UIGridLayout * setSpan(const Sizei& span);

		ElementMode getColumnMode() const;

		UIGridLayout * setColumnMode( const ElementMode& mode );

		ElementMode getRowMode() const;

		UIGridLayout * setRowMode( const ElementMode& mode );

		Float getColumnWeight() const;

		UIGridLayout * setColumnWeight(const Float & columnWeight);

		int getColumnWidth() const;

		UIGridLayout * setColumnWidth(int columnWidth);

		int getRowHeight() const;

		UIGridLayout * setRowHeight(int rowHeight);

		Float getRowWeight() const;

		UIGridLayout * setRowWeight(const Float & rowWeight);

		Rect getPadding() const;

		void setPadding(const Rect & padding);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		Rect mPadding;
		Sizei mSpan;
		ElementMode mColumnMode;
		ElementMode mRowMode;
		Float mColumnWeight;
		int mColumnWidth;
		Float mRowWeight;
		int mRowHeight;

		virtual void onSizeChange();
		
		virtual void onChildCountChange();
		
		virtual void onParentSizeChange( const Vector2f& SizeChange );

		virtual Uint32 onMessage( const UIMessage * Msg );

		Sizef getTargetElementSize();

		void pack();
};

}}

#endif
