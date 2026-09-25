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

/** Original sample selected by hover, before pixel-bucket reduction. */
struct PointTooltipContext {
	size_t index{ 0 };
	double x{ 0.0 };
	double y{ 0.0 };
};

/** Plain hover text. */
struct TooltipText {
	String text;
};

/** One name/value row in a structured hover tooltip. */
struct TooltipField {
	String label;
	String value;
};

/** Structured hover tooltip with an optional title and description. */
struct TooltipData {
	String title;
	SmallVector<TooltipField, 4> fields;
	String description;
};

using TooltipPayload = std::variant<TooltipText, TooltipData>;
/** Return no value to use the default series/value tooltip. Called on hover, not during draw. */
using PointTooltipProvider =
	std::function<std::optional<TooltipPayload>( const PointTooltipContext& )>;

/** Stroke corner treatment. Miter is bounded by LineSeries::miterLimit(). */
enum class LineJoin : Uint8 { Miter, Bevel };

/** Treatment of the first and last point in each continuous run. */
enum class LineCap : Uint8 { Butt, Square };

/** Per-series connection method. MonotoneCubic avoids overshoot between ordered X samples. */
enum class LineInterpolation : Uint8 { Linear, MonotoneCubic };

/** Position along an X tick label that meets the tick after rotation. */
enum class ChartTickLabelAnchor : Uint8 { Start, Center, End };

/** AxisTicks and DataInterval follow the axis; ScreenInterval stays fixed in the plot. */
enum class ChartGridMode : Uint8 { Disabled, AxisTicks, DataInterval, ScreenInterval };

/** One grid direction. AxisTicks chooses nice data values near the requested dp spacing. */
struct ChartGridStyle {
	ChartGridMode mode{ ChartGridMode::Disabled };
	double spacing{ 80.0 };			  //!< Target dp for ticks, axis units for data, dp for screen.
	double offset{ 0.0 };			  //!< Data origin, or dp from the plot's left/top edge.
	Float width{ 1.f };				  //!< Stroke width in dp.
	std::optional<Color> color;		  //!< Unset follows the theme's axis color at reduced opacity.
	const ChartAxis* axis{ nullptr }; //!< Non-owning; null selects the primary axis. A set axis
									  //!< must belong to the model.
};

/**
 * View appearance. Linear measurements use device-independent pixels (dp), including grid spacing
 * in screen mode. Tick spacing is a target: the axis chooses readable intervals near that distance.
 * Colors left unset follow the UI theme. The style does not own its optional font or grid axes.
 */
struct ChartStyle {
	Font* font{ nullptr }; //!< Non-owning; null uses the current UI theme font.
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
	Float xTickLabelAngle{
		0.f }; //!< Clockwise degrees, clamped to [-90, 90]; -90 reads bottom-to-top.
	ChartTickLabelAnchor xTickLabelAnchor{
		ChartTickLabelAnchor::Center }; //!< Label position attached to the X tick after rotation.
	Float yTickSpacing{ 50.f };
	Float hoverDistance{ 10.f };
	Float axisWidth{ 1.f };
	std::optional<Color> axisColor;
	std::optional<Color> tickColor;
	std::optional<Color> tickLabelColor;
	std::optional<Color> axisLabelColor;
	std::optional<Color> hoverColor;
	std::optional<Color> seriesColor;
	ChartGridStyle verticalGrid{ ChartGridMode::Disabled, 80.0 };
	ChartGridStyle horizontalGrid{ ChartGridMode::Disabled, 50.0 };
};

/**
 * One model-owned line. Its axes are borrowed from the same ChartModel; neither this object nor
 * a UIChart owns them separately. A series can share its data source with other series.
 */
class EE_API LineSeries {
  public:
	const String& name() const { return mName; }

	ChartAxis& xAxis() const { return *mXAxis; }

	ChartAxis& yAxis() const { return *mYAxis; }

	/** The axis must belong to this series' model and outlive the series. */
	void setXAxis( ChartAxis& axis );

	/** The axis must belong to this series' model and outlive the series. */
	void setYAxis( ChartAxis& axis );

	void setVisible( bool visible );

	bool visible() const { return mVisible; }

	/** Overrides the theme's default series color. */
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

	/** Changes line geometry; defaults to Linear. */
	void setInterpolation( LineInterpolation interpolation );

	LineInterpolation interpolation() const { return mInterpolation; }

	/** Maximum miter extension as a multiple of half the line width. */
	void setMiterLimit( Float limit );

	Float miterLimit() const { return mMiterLimit; }

	Uint64 geometryRevision() const { return mGeometryRevision; }

	/** Replaces the samples with an owned data source, moving the vector when possible. */
	void setPoints( std::vector<ChartPoint> points );

	/** Copies paired X/Y samples into an owned data source. */
	void setPoints( std::span<const double> xs, std::span<const double> ys );

	/** Copies Y samples with generated X values: start + index * step. */
	void setValues( std::span<const double> ys, double start = 0.0, double step = 1.0 );

	/** Shares ownership of the source; it may be updated independently of the UIChart. */
	void setDataSource( std::shared_ptr<XYDataSource> source );

	std::shared_ptr<XYDataSource> dataSource() const { return mSource; }

	/** Replaces the optional callback used for hover text. */
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
	LineInterpolation mInterpolation{ LineInterpolation::Linear };
	bool mVisible{ true };
	Uint64 mGeometryRevision{ 0 };
};

/**
 * Owns all axes and series. Its default X and Y axes always exist. Added objects are never removed,
 * so references to them remain valid until the model is destroyed. UICharts may share one model
 * while keeping independent ranges, layout, and hover state.
 */
class EE_API ChartModel {
  public:
	ChartModel();

	/** Adds a model-owned axis and returns a borrowed reference. */
	ChartAxis& addAxis( AxisPosition position );

	/** Adds a model-owned series and returns a borrowed reference. */
	LineSeries& addLineSeries( String name );

	/** Borrowed reference to the default bottom axis. */
	ChartAxis& xAxis() const { return *mAxes[0]; }

	/** Borrowed reference to the default left axis. */
	ChartAxis& yAxis() const { return *mAxes[1]; }

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

/**
 * Widget rendering one ChartModel. It owns a shared reference to the model. Views sharing that
 * model still have independent viewport ranges and styles. As with other UIWidgets, the parent
 * scene manages widget lifetime after it is attached to the tree.
 */
class EE_API UIChart : public UIWidget {
  public:
	/** Creates a scene-owned widget using eepp's UIWidget construction convention. */
	static UIChart* New();

	/** Borrowed model reference; the chart always has a model. */
	ChartModel& model() const { return *mModel; }

	/** Owning shared handle for keeping the current model alive or sharing it with another chart.
	 */
	std::shared_ptr<ChartModel> sharedModel() const { return mModel; }

	/** Replaces this view's model. Null creates a new empty model. Old references can expire. */
	void setModel( std::shared_ptr<ChartModel> model );

	/** Returns a borrowed series reference, valid while the current model lives. */
	LineSeries& addLineSeries( String name );

	/** Borrowed reference to the current model's default bottom axis. */
	ChartAxis& xAxis() const { return mModel->xAxis(); }

	/** Borrowed reference to the current model's default left axis. */
	ChartAxis& yAxis() const { return mModel->yAxis(); }

	/** Fits each axis to visible series data and disables X follow mode. */
	void fit();

	/** Returns this view's range for an axis owned by its current model. */
	DataRange axisRange( const ChartAxis& axis );

	/** Sets a manual range on this view; axis must belong to its current model. */
	void setAxisRange( ChartAxis& axis, DataRange range );

	/** Follow the newest X while preserving a fixed data-space span; zero disables follow mode. */
	void setFollowX( double span );

	/** Replaces view appearance and invalidates cached layout and series geometry. */
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
		std::vector<Vector2f> strokePoints;
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
		SmallVector<double, 16> verticalGridTicks;
		SmallVector<double, 16> horizontalGridTicks;
		Float xAxisLabelWidth{ 0.f };
		Float xTickLabelHeight{ 0.f };
		std::vector<RightAxis> rightAxes;
		Uint64 revision{ 0 };
	};

	void synchronize();

	void updateFollow();

	void updateAutoRanges();

	void updateLayout();

	void updateGeometry( SeriesCache& cache, const XYDataRead& read );

	void drawAxes();

	void drawGrid();

	const ChartAxis& gridAxis( const ChartGridStyle& grid, bool vertical ) const;

	void updateHover( Vector2f screenPosition, bool invalidate = true );

	void hideHoverTooltip();

	void updateThemeColors();

	AxisViewport& viewport( const ChartAxis& axis );

	const AxisViewport& viewport( const ChartAxis& axis ) const;

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
