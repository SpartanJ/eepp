#ifndef EE_UI_CHARTS_UICHART_HPP
#define EE_UI_CHARTS_UICHART_HPP

#include <eepp/core/observablevalue.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/ui/charts/chartaxis.hpp>
#include <eepp/ui/charts/chartreduction.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <functional>
#include <limits>

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI { namespace Charts {

using namespace EE::Graphics;

struct PointTooltipContext {
	size_t index{ 0 };
	double x{ 0.0 };
	double y{ 0.0 };
};

struct TooltipText {
	String text;
};

struct TooltipField {
	String label;
	String value;
};

struct TooltipData {
	String title;
	SmallVector<TooltipField, 4> fields;
	String description;
};

using TooltipPayload = std::variant<TooltipText, TooltipData>;
using PointTooltipProvider =
	std::function<std::optional<TooltipPayload>( const PointTooltipContext& )>;

enum class LineJoin : Uint8 { Miter, Bevel };
enum class LineCap : Uint8 { Butt, Square };

/** Chart measurements are in device-independent pixels. Colors left unset follow the UI theme. */
struct ChartStyle {
	Font* font{ nullptr }; //!< Null uses the current UI theme font.
	Float fontSize{ 12.f };
	Float tickLength{ 4.f };
	Float tickLabelGap{ 6.f };
	Float axisLabelGap{ 25.f };
	Float axisLabelInset{ 2.f };
	Float leftMargin{ 50.f };
	Float rightMargin{ 14.f };
	Float topMargin{ 12.f };
	Float topMarginWithLabel{ 26.f };
	Float bottomMargin{ 34.f };
	Float bottomMarginWithLabel{ 52.f };
	Float minimumAxisMargin{ 42.f };
	Float axisLabelPadding{ 12.f };
	Float rightAxisLabelPadding{ 16.f };
	Float xTickSpacing{ 80.f };
	Float yTickSpacing{ 50.f };
	Float hoverDistance{ 10.f };
	Float axisWidth{ 1.f };
	std::optional<Color> axisColor;
	std::optional<Color> tickColor;
	std::optional<Color> tickLabelColor;
	std::optional<Color> axisLabelColor;
	std::optional<Color> hoverColor;
	std::optional<Color> seriesColor;
};

class EE_API LineSeries {
  public:
	const String& name() const { return mName; }

	ChartAxis* xAxis() const { return mXAxis; }

	ChartAxis* yAxis() const { return mYAxis; }

	void setXAxis( ChartAxis* axis );

	void setYAxis( ChartAxis* axis );

	void setVisible( bool visible );

	bool visible() const { return mVisible; }

	void setColor( Color color );

	Color color() const { return mColor; }

	bool hasCustomColor() const { return mColorExplicit; }

	/** Stroke width in device-independent pixels. */
	void setWidth( Float width );

	Float width() const { return mWidth; }

	void setJoin( LineJoin join );

	LineJoin join() const { return mJoin; }

	void setCap( LineCap cap );

	LineCap cap() const { return mCap; }

	void setMiterLimit( Float limit );

	Float miterLimit() const { return mMiterLimit; }

	Uint64 geometryRevision() const { return mGeometryRevision; }

	void setPoints( std::vector<ChartPoint> points );

	void setPoints( std::span<const double> xs, std::span<const double> ys );

	void setValues( std::span<const double> ys, double start = 0.0, double step = 1.0 );

	void setDataSource( std::shared_ptr<XYDataSource> source );

	std::shared_ptr<XYDataSource> dataSource() const { return mSource; }

	void setTooltipProvider( PointTooltipProvider provider ) {
		mTooltipProvider = std::move( provider );
		changed();
	}

	const PointTooltipProvider& tooltipProvider() const { return mTooltipProvider; }

  private:
	friend class ChartModel;
	LineSeries( String name, ChartAxis* xAxis, ChartAxis* yAxis, std::function<void()> changed );
	void changed() { mChanged(); }

	String mName;
	std::shared_ptr<XYDataSource> mSource;
	std::function<void()> mChanged;
	PointTooltipProvider mTooltipProvider;
	ChartAxis* mXAxis;
	ChartAxis* mYAxis;
	Color mColor{ 48, 130, 220 };
	bool mColorExplicit{ false };
	Float mWidth{ 2.f };
	Float mMiterLimit{ 4.f };
	LineJoin mJoin{ LineJoin::Miter };
	LineCap mCap{ LineCap::Butt };
	bool mVisible{ true };
	Uint64 mGeometryRevision{ 0 };
};

/** Series and axis semantics, shareable by independent UIChart viewports. */
class EE_API ChartModel {
  public:
	ChartModel();

	ChartAxis* addAxis( AxisPosition position );

	LineSeries* addLineSeries( String name );

	ChartAxis* xAxis() const { return mAxes[0].get(); }

	ChartAxis* yAxis() const { return mAxes[1].get(); }

	const SmallVector<std::unique_ptr<ChartAxis>, 4>& axes() const { return mAxes; }

	const SmallVector<std::unique_ptr<LineSeries>, 2>& series() const { return mSeries; }

	Uint64 revision() const { return mRevision.get(); }

  private:
	friend class UIChart;
	void changed() { mRevision.set( mRevision.get() + 1 ); }

	SmallVector<std::unique_ptr<ChartAxis>, 4> mAxes;
	SmallVector<std::unique_ptr<LineSeries>, 2> mSeries;
	ObservableValue<Uint64> mRevision{ 0 };
};

class EE_API UIChart : public UIWidget {
  public:
	static UIChart* New();

	ChartModel* model() const { return mModel.get(); }

	std::shared_ptr<ChartModel> sharedModel() const { return mModel; }

	void setModel( std::shared_ptr<ChartModel> model );

	LineSeries* addLineSeries( String name );

	ChartAxis* xAxis() const { return mModel->xAxis(); }

	ChartAxis* yAxis() const { return mModel->yAxis(); }

	void fit();

	DataRange axisRange( const ChartAxis* axis );

	void setAxisRange( ChartAxis* axis, DataRange range );

	/** Follow the newest X while preserving a fixed data-space span; zero disables follow mode. */
	void setFollowX( double span );

	void setChartStyle( ChartStyle style );

	const ChartStyle& chartStyle() const { return mChartStyle; }

	void draw() override;

  protected:
	UIChart();

	void onSizeChange() override;

	void scheduledUpdate( const Time& time ) override;

	Uint32 onMouseDown( const Vector2i& position, const Uint32& flags ) override;

	Uint32 onMouseUp( const Vector2i& position, const Uint32& flags ) override;

	Uint32 onMouseMove( const Vector2i& position, const Uint32& flags ) override;

	Uint32 onMouseOver( const Vector2i& position, const Uint32& flags ) override;

	Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags ) override;

	Uint32 onMouseWheel( const Vector2f& offset, bool flipped ) override;

  private:
	struct ViewAxis {
		ChartAxis* axis;
		AxisViewport viewport;
		Uint64 axisRevision{ 0 };
	};

	struct SeriesCache {
		LineSeries* series{ nullptr };
		VertexBufferUniquePtr geometry;
		Uint64 generation{ 0 };
		Uint64 viewRevision{ 0 };
		Uint64 seriesRevision{ 0 };
		std::vector<ReducedPoint> reduced;
	};

	struct Layout {
		struct RightAxis {
			ChartAxis* axis{ nullptr };
			SmallVector<double, 16> ticks;
			SmallVector<String, 8> labels;
			Float width{ 0.f };
		};

		Rectf plot;
		SmallVector<double, 16> xTicks;
		SmallVector<double, 16> yTicks;
		SmallVector<String, 8> xLabels;
		SmallVector<String, 8> yLabels;
		SmallVector<Float, 8> xLabelWidths;
		SmallVector<Float, 8> yLabelWidths;
		Float xAxisLabelWidth{ 0.f };
		std::vector<RightAxis> rightAxes;
		Uint64 revision{ 0 };
	};

	void synchronize();

	void updateFollow();

	void updateAutoRanges();

	void updateLayout();

	void updateGeometry( SeriesCache& cache, const XYDataRead& read );

	void drawAxes();

	void updateHover( Vector2f screenPosition );

	void hideHoverTooltip();

	void updateThemeColors();

	AxisViewport& viewport( const ChartAxis* axis );

	const AxisViewport& viewport( const ChartAxis* axis ) const;

	std::shared_ptr<ChartModel> mModel;
	ObservableValue<Uint64>::Connection mModelConnection;
	SmallVector<ViewAxis, 4> mAxes;
	SmallVector<SeriesCache, 2> mCaches;
	Layout mLayout;
	Uint64 mViewRevision{ 1 };
	Uint64 mLayoutRevision{ 1 };
	Vector2f mLastMouse;
	LineSeries* mHoverSeries{ nullptr };
	size_t mHoverIndex{ 0 };
	Uint64 mHoverGeneration{ 0 };
	Uint64 mHoverRevision{ 0 };
	bool mHover{ false };
	bool mPanning{ false };
	double mFollowSpan{ 0.0 };
	Uint64 mFollowRevision{ 0 };
	Uint64 mAutoRevision{ std::numeric_limits<Uint64>::max() };
	String mLastXAxisLabel;
	String mLastYAxisLabel;
	Font* mLastFont{ nullptr };
	ChartStyle mChartStyle;
	Color mAxisColor{ 125, 130, 138 };
	Color mTickColor{ 125, 130, 138 };
	Color mTickLabelColor{ 190, 194, 201 };
	Color mAxisLabelColor{ 210, 214, 220 };
	Color mHoverColor{ 160, 165, 173, 140 };
	Color mSeriesColor{ 48, 130, 220 };
	Float mLastDensity{ 0.f };
};

}}} // namespace EE::UI::Charts

#endif
