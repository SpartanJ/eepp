#ifndef EE_UI_FLEXLAYOUTER_HPP
#define EE_UI_FLEXLAYOUTER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/csslayouttypes.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uilayouter.hpp>

namespace EE { namespace UI {

class UIWidget;
class UIHTMLWidget;

class EE_API FlexLayouter : public UILayouter {
  public:
	struct FlexItem {
		UIWidget* widget;
		int order;
		Float flexGrow;
		Float flexShrink;
		Float flexBasisValue;
		bool flexBasisAuto;
		bool flexBasisContent;
		bool flexBasisIsPercentage;
		std::string flexBasisRaw;

		CSSAlignSelf alignSelf;

		Float outerMainSize;
		Float outerCrossSize;
		Float targetMainSize;
		Float crossSize;
		Float mainPos;
		Float crossPos;

		// Margins along the flex axes
		Float marginMainStart{ 0.f };
		Float marginMainEnd{ 0.f };
		Float marginCrossStart{ 0.f };
		Float marginCrossEnd{ 0.f };

		Float minMainSize{ 0.f };
		Float maxMainSize{ std::numeric_limits<Float>::max() };
		bool frozen{ false };
		bool hasAutoMarginMainStart{ false };
		bool hasAutoMarginMainEnd{ false };
		bool hasAutoMarginCrossStart{ false };
		bool hasAutoMarginCrossEnd{ false };
		bool collapsed{ false };
		Float savedCrossSize{ 0.f };

		FlexItem() :
			widget( nullptr ),
			order( 0 ),
			flexGrow( 0.f ),
			flexShrink( 1.f ),
			flexBasisValue( 0.f ),
			flexBasisAuto( true ),
			flexBasisContent( false ),
			flexBasisIsPercentage( false ),
			alignSelf( CSSAlignSelf::Auto ),
			outerMainSize( 0.f ),
			outerCrossSize( 0.f ),
			targetMainSize( 0.f ),
			crossSize( 0.f ),
			mainPos( 0.f ),
			crossPos( 0.f ),
			collapsed( false ),
			savedCrossSize( 0.f ) {}
	};

	struct FlexLine {
		SmallVector<size_t, 16> itemIndices;
		Float mainSize;
		Float crossSize;
		Float crossPos;

		FlexLine() : mainSize( 0.f ), crossSize( 0.f ), crossPos( 0.f ) {}
	};

	FlexLayouter( UIWidget* container ) : UILayouter( container ) {}

	void updateLayout() override;
	void computeIntrinsicWidths() override;
	Float getMinIntrinsicWidth() override;
	Float getMaxIntrinsicWidth() override;

	Float getBaseline() const { return mContainerBaseline; }

  protected:
	struct Axis {
		bool horizontal;
		bool reversed;
	};

	void collectFlexItems( SmallVector<FlexItem, 16>& items );

	void readContainerStyle( CSSFlexDirection& direction, CSSFlexWrap& wrap,
							 CSSJustifyContent& justify, CSSAlignItems& alignItems,
							 CSSAlignContent& alignContent, Float& columnGap, Float& rowGap );

	void readItemStyle( UIWidget* child, FlexItem& item );

	Axis getMainAxis( CSSFlexDirection direction ) const;

	Axis getCrossAxis( CSSFlexDirection direction ) const;

	Float resolveFlexBasis( UIWidget* child, CSSFlexDirection direction, Float flexBasisValue,
							bool flexBasisAuto, const Axis& mainAxis,
							bool flexBasisContent = false );

	Float getItemMainSize( UIWidget* child, const Axis& mainAxis ) const;

	Float getItemCrossSize( UIWidget* child, const Axis& crossAxis ) const;

	void generateFlexLines( SmallVector<FlexLine, 8>& lines, const Float containerMainSize,
							const Float columnGap );

	void resolveFlexibleLengths( FlexLine& line, const Float containerMainSize,
								 const Float columnGap );

	void alignMainAxis( FlexLine& line, const Float containerMainSize, const Float columnGap );

	void resolveCrossSizes( FlexLine& line, const Axis& crossAxis, const Axis& mainAxis );

	void alignCrossAxis( const SmallVector<FlexLine, 8>& lines, const Float containerCrossSize,
						 const Float rowGap, const Axis& crossAxis );

	void applyLayout( const SmallVector<FlexLine, 8>& lines, const Axis& mainAxis,
					  const Axis& crossAxis, const Rectf& containerPadding,
					  const Float containerWidth, const Float containerHeight,
					  const SizePolicy widthPolicy, const SizePolicy heightPolicy );

	void measureFlexItems( const Axis& mainAxis, const Axis& crossAxis,
						   const Float containerCrossSize, const Float containerWidth,
						   const Float containerHeight, const Rectf& containerPadding,
						   bool indefiniteMainSize, bool indefiniteCrossSize );

	SmallVector<FlexItem, 16> mItems;
	Float mContainerBaseline{ 0.f };
	CSSFlexDirection mDirection{ CSSFlexDirection::Row };
	CSSFlexWrap mWrap{ CSSFlexWrap::NoWrap };
	CSSJustifyContent mJustify{ CSSJustifyContent::FlexStart };
	CSSAlignItems mAlignItems{ CSSAlignItems::Stretch };
	CSSAlignContent mAlignContent{ CSSAlignContent::Stretch };
	Float mColumnGap{ 0.f };
	Float mRowGap{ 0.f };
};

}} // namespace EE::UI

#endif
