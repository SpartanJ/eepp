# UI Charts

## Overview

`UIChart` is eepp's interactive, hardware-accelerated line chart widget. It supports multiple
series and Y axes, live data, pan and zoom, hover tooltips, configurable grids, smooth curves,
and rotated X-axis labels. Charts can be created in C++ or placed in a UI XML layout as `<Chart>`.

The public API is available through `<eepp/ui.hpp>`. The individual headers are under
`<eepp/ui/charts/>`, and the types live in `EE::UI::Charts`. The complete runnable example is
[`src/examples/ui_charts/ui_charts.cpp`](https://github.com/SpartanJ/eepp/blob/develop/src/examples/ui_charts/ui_charts.cpp).

The module currently draws line series on linear axes. X and Y values are numeric `double`s;
formatters decide how ticks appear, including dates or units. A chart does not convert date strings
or other application data into numeric coordinates for you.

## A first chart

This example assumes an initialized `UISceneNode`, such as `app.getUI()` in a `UIApplication`:

```cpp
#include <eepp/ui.hpp>

using namespace EE::UI;
using namespace EE::UI::Charts;

auto* chart = UIChart::New();
chart->setParent( ui->getRoot() );
chart->setSize( 500, 300 );
chart->xAxis().setLabel( "Second" );
chart->yAxis().setLabel( "Temperature (C)" );

auto& temperature = chart->addLineSeries( "Temperature" );
temperature.setColor( Color( 255, 113, 132 ) );
temperature.setWidth( 2.f ); // Device-independent pixels.
temperature.setPoints( std::vector<ChartPoint>{
    { 0.0, 20.0 }, { 1.0, 21.5 }, { 2.0, 21.0 }, { 3.0, 23.0 }
} );
chart->fit();
```

`ChartPoint` contains `double x` and `double y`. `setPoints(std::vector<ChartPoint>)` creates an
owned data source and moves the vector into it when possible. There are also convenience overloads:

```cpp
series.setPoints( xs, ys );              // Copies paired spans of double values.
series.setValues( ys, 10.0, 0.5 );        // Copies Y values; X is 10 + index * 0.5.
```

The paired overload copies up to the length of the shorter span. Supply equal-length spans when
every sample matters. If the data already lives in another representation, use one of the data
sources described below to avoid an unnecessary point copy.

### Chart in an XML layout

`Chart` is registered with the standard widget creator. The layout controls the widget's size;
configure its model and chart-specific style from C++ after loading it:

```xml
<vbox lw="mp" lh="mp" padding="8dp">
    <TextView text="Temperature history" lw="mp" lh="wc" />
    <Chart id="temperature-chart" lw="mp" lh="0" lw8="1" />
</vbox>
```

```cpp
auto* layout = ui->loadLayoutFromString( xml );
auto* chart = layout->find<UIChart>( "temperature-chart" );
chart->addLineSeries( "Temperature" ).setPoints( std::move( points ) );
chart->fit();
```

The `lh="0" lh8="1"` combination gives the chart the remaining height in a `vbox`, so it grows
with its parent. Standard widget CSS can set its background, padding, and other widget properties.
Chart-specific drawing options are set through `ChartStyle`.

## Model, series, and axes

Every `UIChart` starts with a `ChartModel`, a bottom X axis, and a left Y axis. The model owns its
axes and line series. `addLineSeries()`, `addAxis()`, `xAxis()`, and `yAxis()` return borrowed
references to those model-owned objects. Added axes and series remain at stable addresses until the
model is destroyed. Keep a `std::shared_ptr<ChartModel>` when their lifetime must extend beyond a
particular widget.

```cpp
auto& load = chart->addLineSeries( "Load" );
load.setPoints( std::move( loadPoints ) );

auto& rightAxis = chart->model().addAxis( AxisPosition::Right );
rightAxis.setLabel( "Temperature (C)" );
auto& ambient = chart->addLineSeries( "Ambient" );
ambient.setYAxis( rightAxis );
ambient.setPoints( std::move( ambientPoints ) );
chart->fit();
```

New series use the model's default X and Y axes. `setXAxis()` and `setYAxis()` accept references to
axes owned by the **same model**. `ChartAxis::setLabel()` stores a `String`, and
`setFormatter()` customizes tick text:

```cpp
chart->yAxis().setFormatter( []( double value, double ) {
    return String::format( "%.0f%%", value );
} );
```

The formatter receives the tick value and selected step. The default formatter selects readable
decimal or scientific notation. Tick values are chosen automatically to approach the requested
visual spacing; a formatter changes their text, not their positions.

To display the same data in separate viewports, share the model:

```cpp
auto shared = chart->sharedModel();
otherChart->setModel( shared );
otherChart->setAxisRange( otherChart->xAxis(), { 50.0, 100.0 } );
```

Each widget retains its own visible ranges, style, layout, and hover state. `setModel(nullptr)`
creates a fresh empty model. References obtained from the old model may then expire unless another
`shared_ptr` keeps it alive.

### Ranges and constraints

`fit()` fits every axis to the visible series data and turns off X follow mode. Without a manual
range, axes fit their data automatically as sources change. You can set a range for a particular
chart view and read it back:

```cpp
chart->setAxisRange( chart->xAxis(), { 0.0, 60.0 } );
DataRange visibleX = chart->axisRange( chart->xAxis() );
```

Setting a range makes that axis manual. `fit()` returns it to automatic fitting. To limit pan,
zoom, and fitted ranges, configure the axis itself:

```cpp
RangeConstraints limits;
limits.absoluteMinimum = 0.0;
limits.absoluteMaximum = 100.0;
limits.minimumViewSpan = 5.0;
limits.maximumViewSpan = 100.0; // Zero means unbounded.
chart->yAxis().setConstraints( limits );
```

Constraints belong to the model's axis and therefore apply to every view sharing that model.
Visible ranges belong to each `UIChart`.

## Data sources

`LineSeries::setDataSource(std::shared_ptr<XYDataSource>)` accepts a reusable source and retains a
shared reference. Several series can share one source, including across charts. Choose a source
according to how data is stored and updated:

| Source | Use | Storage and updates |
| --- | --- | --- |
| `OwnedXYDataSource` | An editable sequence of `ChartPoint` values | Owns the vector; `reserve()`, `setPoints()`, `append()`, and `clear()` |
| `RingXYDataSource` | Bounded live history | Fixed-capacity FIFO; oldest points are discarded on overflow |
| `ImmutableXYDataSource` | Existing immutable arrays or records | Shares backing storage without copying samples |
| `ModelXYDataSource` | Numeric columns of an eepp `Models::Model` | Copies model values into a typed cache when the model updates |

For a live chart, retain the source and append on a UI timer:

```cpp
auto ring = std::make_shared<RingXYDataSource>( 600 );
auto& live = chart->addLineSeries( "CPU" );
live.setDataSource( ring );
chart->setFollowX( 60.0 ); // Keep a 60-unit X window at the newest sample.

ui->setInterval( [ring, nextX = 0.0]() mutable {
    const double value = readCpuLoad();
    ring->append( { nextX++, value } );
}, Milliseconds( 100 ), String::hash( "cpu-chart-update" ) );
```

Here `readCpuLoad()` stands for an application-provided sampler. Cancel the timer when the
associated screen is removed if the application keeps its UI scene alive.

Keep X values increasing for the optimized visible-range lookup and pixel-bucket reduction. In a
ring source, a wrapped read exposes two logical chunks in the right order; callers do not need to
rearrange samples. `setFollowX(span)` uses the newest X value on the primary X axis and refits Y
axes to the visible window as data changes. Pass zero to stop following. Manual range changes,
mouse pan, mouse zoom, and `fit()` also stop follow mode. Call `setFollowX()` again to resume it.

For immutable X and Y arrays, keep them in shared vectors and create a zero-copy source:

```cpp
auto xs = std::make_shared<const std::vector<double>>( std::move( times ) );
auto ys = std::make_shared<const std::vector<Float>>( std::move( values ) );
series.setDataSource( makeArrayXYDataSource( xs, ys ) );
```

`makeMemberXYDataSource()` similarly reads numeric members of a shared vector of standard-layout
records. Its X and Y members can be `float` or `double`. Treat storage behind an immutable source
as immutable for the source's whole lifetime. `ModelXYDataSource` accepts a shared
`Models::Model`, X and Y column indexes, and optional model roles; nonnumeric values become gaps.
Its cache is rebuilt when the model reports an update, so it is most useful when model integration
matters more than avoiding a copy on each update.

Custom sources implement `XYDataSource::acquireRead()` and publish a coherent `XYDataRead` with a
generation, data order, and one or more `XYDataChunk`s. A read's spans and backing token must stay
valid until that read is destroyed. For mutable storage, writers must be synchronized with active
readers. `DataDelta` can describe an append or reset for consumers that can update incrementally.
The built-in mutable sources use locks and release them before notifying listeners. Keep read
transactions short so producers are not blocked by a long-held read lock.

Nonfinite X or Y values (`NaN` or infinity) form gaps rather than connected line segments. An
ascending X order is required for the fast reduction path; out-of-order X data does not receive
that reduction. If plotting becomes unexpectedly slow or empty, inspect the source's
`XYDataRead::order()` and the X values.

## Appearance

Copy the current `ChartStyle`, change the desired fields, and pass the full style back. The chart
keeps its own copy. Lengths such as margins, font size, tick spacing, line width, hover distance,
and grid screen spacing are device-independent pixels (dp), scaled by eepp's `PixelDensity`.

```cpp
ChartStyle style = chart->chartStyle();
style.fontSize = 13.f;
style.xTickSpacing = 96.f;
style.tickLabelColor = Color( 210, 214, 220 );
style.axisColor = Color( 125, 130, 138 );
chart->setChartStyle( style );

series.setColor( Color( 83, 179, 255 ) );
series.setWidth( 3.f );
series.setJoin( LineJoin::Bevel );
series.setCap( LineCap::Square );
series.setMiterLimit( 4.f );
series.setVisible( true );
```

| `ChartStyle` group | Fields |
| --- | --- |
| Text | `font`, `fontSize`, `tickLabelColor`, `axisLabelColor` |
| Margins and text gaps | `leftMargin`, `rightMargin`, `topMargin`, `topMarginWithLabel`, `bottomMargin`, `bottomMarginWithLabel`, `minimumAxisMargin`, `axisLabelPadding`, `rightAxisLabelPadding`, `axisLabelGap`, `axisLabelInset`, `tickLabelGap` |
| Ticks | `tickLength`, `xTickSpacing`, `yTickSpacing`, `xTickLabelAngle`, `xTickLabelAnchor` |
| Strokes and hover | `axisWidth`, `axisColor`, `tickColor`, `hoverColor`, `hoverDistance`, `seriesColor` |
| Grid | `verticalGrid`, `horizontalGrid` |

An unset optional color follows the current UI theme. The chart uses the theme's `--tab-line`
for axes, `--font-hint` for tick text, `--font` for axis labels, and `--primary` for its default
series and hover colors, with built-in fallback colors. Tick color defaults to the resolved axis
color. Grid color defaults to the axis color with reduced opacity. `ChartStyle::font == nullptr`
uses the UI theme's default font. The optional `font` pointer is borrowed; its font must outlive
the chart's use of it. A series's explicit `setColor()` overrides the style's default series color.

### Grids

Vertical and horizontal grids are configured independently. Both start disabled.

| `ChartGridMode` | Positioning | `spacing` unit | Effect of pan and zoom |
| --- | --- | --- | --- |
| `Disabled` | No lines | — | — |
| `AxisTicks` | Nice data ticks near a target distance | dp | Follows the selected axis |
| `DataInterval` | Exact data values at `offset + n * spacing` | Axis units | Follows the selected axis |
| `ScreenInterval` | Fixed positions measured from the plot's left or top edge | dp | Stays fixed in the plot |

`offset` uses axis units for `DataInterval`, dp for `ScreenInterval`, and has no effect on
`AxisTicks`. `width` is in dp. `color` is optional. `axis` is a **non-owning** pointer to an axis
in the same model; null uses the primary X or Y axis. Select an X-oriented axis for vertical
lines and a Y-oriented axis for horizontal lines:

```cpp
ChartStyle style = chart->chartStyle();
style.verticalGrid.mode = ChartGridMode::DataInterval;
style.verticalGrid.spacing = 10.0;
style.verticalGrid.offset = 0.0;
style.verticalGrid.width = 1.f;
style.verticalGrid.color = Color( 120, 130, 150, 64 );
style.horizontalGrid.mode = ChartGridMode::AxisTicks;
style.horizontalGrid.spacing = 48.0; // Target gap in dp.
chart->setChartStyle( style );
```

The plot clips grid lines to its bounds. Very dense configurations are bounded to avoid drawing
an unbounded number of lines; give `spacing` a positive finite value.

### Smoothed lines and angled tick labels

Interpolation is selected per series. `Linear` joins samples with straight segments.
`MonotoneCubic` smooths ordered X samples without overshooting adjacent sample Y values. Gaps
break the curve, and the hover hit test follows the smoothed stroke. Use linear interpolation
when straight edges or exact piecewise-linear values are desired.

```cpp
series.setInterpolation( LineInterpolation::MonotoneCubic );

ChartStyle style = chart->chartStyle();
style.xTickLabelAngle = -45.f;
style.xTickLabelAnchor = ChartTickLabelAnchor::End;
chart->setChartStyle( style );
chart->xAxis().setFormatter( []( double day, double ) {
    return String::format( "Day %.0f", day );
} );
```

`xTickLabelAngle` is in degrees, clamped to `[-90, 90]`; negative values tilt labels toward
bottom-to-top reading. The anchor (`Start`, `Center`, or `End`) chooses which part of the rotated
label meets its tick. The layout reserves room for the rotated text automatically. Formatting
dates is an application concern: represent time as a numeric X coordinate and return the desired
date text from the axis formatter.

## Hover tooltips and interaction

Moving the cursor near a visible line highlights its X position and displays a tooltip for an
**original source sample**, rather than an interpolated curve point. This also holds for a
pixel-bucket-reduced line. The default tooltip contains the series name and formatted X and Y
values. Set a provider
on a series to return plain or structured text:

```cpp
series.setTooltipProvider(
    []( const PointTooltipContext& point ) -> std::optional<TooltipPayload> {
        TooltipData tooltip;
        tooltip.title = "Sample";
        tooltip.fields.emplace_back(
            TooltipField{ "Index", String::toString( static_cast<Uint64>( point.index ) ) } );
        tooltip.fields.emplace_back(
            TooltipField{ "Value", String::format( "%.2f", point.y ) } );
        tooltip.description = "Measured value";
        return tooltip;
    } );
```

`PointTooltipContext::index` is the current logical index in the source read. A ring's indexes
shift as old samples are discarded; use the sample's X value for a stable time or sequence number.
Returning `std::nullopt` uses the default tooltip. Returning an empty `TooltipText` suppresses
visible tooltip text. Providers run when hover changes, not for every line drawn. An updating
chart rechecks hover after its geometry changes, so a stationary cursor follows live samples.

| Input | Result |
| --- | --- |
| Hover near a series | Show the nearest sample's tooltip and an X guide line |
| Drag with the left mouse button | Pan the visible ranges; stop X follow mode |
| Mouse wheel over the plot | Zoom around the cursor; stop X follow mode |
| `fit()` from application code | Fit visible series and stop X follow mode |

Hover is limited by `ChartStyle::hoverDistance` in dp and by the plot area. The tooltip is hidden
when the cursor leaves, the selected series disappears, or its text is empty. Chart interactions
apply to all axes in that view; an axis's `RangeConstraints` still limit the resulting ranges.

## Dense data and update costs

For ascending X data, the chart finds the visible interval and reduces it by screen pixel columns.
Each bucket preserves its first, last, minimum, and maximum source samples in source order, so
narrow spikes remain visible. Reduced points retain source indexes for hover. A series with a
small visible point count is drawn directly. Geometry and layout buffers are reused and rebuilt
when their data generation, viewport, series style, or size changes.

Keep frequent data updates in a mutable source instead of replacing a large vector each frame.
Prefer `RingXYDataSource` for bounded histories and immutable shared arrays for existing static
data. Smooth interpolation adds stroke segments and work after reduction, so linear interpolation
is generally the better starting point for very dense traces. Axis formatters and tooltip
providers can allocate strings; formatters run during layout and providers run on hover, not for
each sample in the render loop.

For a hands-on reference, run the [charts example](https://github.com/SpartanJ/eepp/blob/develop/src/examples/ui_charts/ui_charts.cpp).
It shows a responsive four-chart layout with rotated date-like labels, smooth and linear lines,
multiple Y axes, grid modes, structured tooltips, a 250,000-point trace, and a live ring. Hover or
drag a chart, scroll to zoom, press `R` to fit all views, `Space` to append samples, and `C` or
`T` to toggle a series.
