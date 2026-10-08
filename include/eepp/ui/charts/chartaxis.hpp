#ifndef EE_UI_CHARTS_CHARTAXIS_HPP
#define EE_UI_CHARTS_CHARTAXIS_HPP

#include <eepp/core/string.hpp>
#include <eepp/ui/charts/xydatasource.hpp>
#include <functional>
#include <utility>

namespace EE { namespace UI { namespace Charts {

enum class AxisPosition : Uint8 { Bottom, Top, Left, Right };

/** Future scales can override these two operations without changing viewport math. */
class EE_API AxisScale {
  public:
	virtual ~AxisScale() = default;

	virtual double forward( double value ) const = 0;

	virtual double inverse( double value ) const = 0;
};

class EE_API LinearScale final : public AxisScale {
  public:
	double forward( double value ) const override { return value; }

	double inverse( double value ) const override { return value; }
};

struct RangeConstraints {
	std::optional<double> absoluteMinimum;
	std::optional<double> absoluteMaximum;
	double minimumViewSpan{ 0.0 };
	double maximumViewSpan{ 0.0 }; // Zero means unbounded.
};

/** Axis semantics are independent of a UIChart's per-view viewport. */
class EE_API ChartAxis {
  public:
	using Formatter = std::function<String( double value, double step )>;

	explicit ChartAxis( AxisPosition position, std::function<void()> changed = {} ) :
		mChanged( std::move( changed ) ), mPosition( position ) {}

	AxisPosition position() const { return mPosition; }

	void setLabel( String label ) {
		if ( mLabel != label ) {
			mLabel = std::move( label );
			changed();
		}
	}

	const String& label() const { return mLabel; }

	void setFormatter( Formatter formatter ) {
		mFormatter = std::move( formatter );
		changed();
	}

	String formatTick( double value, double step ) const;

	void setConstraints( RangeConstraints constraints ) {
		mConstraints = constraints;
		changed();
	}

	const RangeConstraints& constraints() const { return mConstraints; }

	const AxisScale& scale() const { return mScale; }

	Uint64 revision() const { return mRevision; }

  private:
	void changed() {
		++mRevision;
		if ( mChanged )
			mChanged();
	}

	String mLabel;
	Formatter mFormatter;
	std::function<void()> mChanged;
	RangeConstraints mConstraints;
	AxisPosition mPosition;
	LinearScale mScale;
	Uint64 mRevision{ 0 };
};

/** Mutable, per-view range in transformed coordinates. */
class EE_API AxisViewport {
  public:
	AxisViewport() = default;

	DataRange range() const { return mRange; }

	bool manual() const { return mManual; }

	void fit( std::optional<DataRange> extent, const ChartAxis& axis );

	void setRange( DataRange range, const ChartAxis& axis );

	void zoom( double anchor, double factor, const ChartAxis& axis );

	void pan( double delta, const ChartAxis& axis );

	double toPixel( double value, float pixelStart, float pixelLength, bool inverted,
					const ChartAxis& axis ) const;

	double fromPixel( float pixel, float pixelStart, float pixelLength, bool inverted,
					  const ChartAxis& axis ) const;

  private:
	void apply( DataRange transformed, const ChartAxis& axis, bool manual );

	DataRange mRange{ 0.0, 1.0 };
	bool mManual{ false };
};

/** Nice-number ticks, independent of label formatting and text measurement. */
EE_API SmallVector<double, 16> linearTicks( DataRange range, float pixelLength,
											float targetSpacing = 80.f );

EE_API String formatNumericTick( double value, double step );

}}} // namespace EE::UI::Charts

#endif
