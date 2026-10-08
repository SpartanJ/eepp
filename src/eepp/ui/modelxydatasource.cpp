#include <eepp/system/lock.hpp>
#include <eepp/ui/charts/modelxydatasource.hpp>
#include <limits>

namespace EE { namespace UI { namespace Charts {

namespace {

double numericValue( const Models::Variant& value ) {
	switch ( value.getType() ) {
		case Models::Variant::Type::Float:
			return value.asFloat();
		case Models::Variant::Type::Int:
			return value.asInt();
		case Models::Variant::Type::Uint:
			return value.asUint();
		case Models::Variant::Type::Int64:
			return static_cast<double>( value.asInt64() );
		case Models::Variant::Type::Uint64:
			return static_cast<double>( value.asUint64() );
		default:
			return std::numeric_limits<double>::quiet_NaN();
	}
}

} // namespace

ModelXYDataSource::ModelXYDataSource( std::shared_ptr<Models::Model> model, size_t xColumn,
									  size_t yColumn, Models::ModelRole xRole,
									  Models::ModelRole yRole ) :
	mModel( std::move( model ) ),
	mXColumn( xColumn ),
	mYColumn( yColumn ),
	mXRole( xRole ),
	mYRole( yRole ) {
	if ( mModel ) {
		mModel->registerClient( this );
		refresh();
	}
}

ModelXYDataSource::~ModelXYDataSource() {
	if ( mModel )
		mModel->unregisterClient( this );
}

void ModelXYDataSource::refresh() {
	if ( !mModel )
		return;
	std::vector<ChartPoint> points;
	{
		System::Lock lock( mModel->resourceMutex() );
		if ( mXColumn < mModel->columnCount() && mYColumn < mModel->columnCount() ) {
			const size_t count = mModel->rowCount();
			points.reserve( count );
			for ( size_t row = 0; row < count; ++row ) {
				const auto xIndex =
					mModel->index( static_cast<int>( row ), static_cast<int>( mXColumn ) );
				const auto yIndex =
					mModel->index( static_cast<int>( row ), static_cast<int>( mYColumn ) );
				points.emplace_back( ChartPoint{ numericValue( mModel->data( xIndex, mXRole ) ),
												 numericValue( mModel->data( yIndex, mYRole ) ) } );
			}
		}
	}
	mCache.setPoints( std::move( points ) );
	Uint64 generation;
	{
		auto read = mCache.acquireRead();
		generation = read.generation();
	}
	notifyChanged( generation );
}

void ModelXYDataSource::onModelUpdated( unsigned ) {
	refresh();
}

}}} // namespace EE::UI::Charts
