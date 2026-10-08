#ifndef EE_UI_MODELS_OBSERVABLELISTMODEL_HPP
#define EE_UI_MODELS_OBSERVABLELISTMODEL_HPP

#include <eepp/core/observablevector.hpp>
#include <eepp/ui/models/model.hpp>
#include <functional>
#include <optional>
#include <type_traits>

namespace EE { namespace UI { namespace Models {

/**
 * @brief One-column model adapter for an ObservableVector.
 *
 * Unfiltered sources preserve incremental model notifications. A filtered projection rebuilds its
 * row mapping when source membership can change. Custom formatters allow domain types to remain
 * independent from Variant. The model retains the source storage, so it remains safe when a view
 * outlives the ObservableVector wrapper. Mutations naturally stop when that wrapper is destroyed.
 *
 * Filtering changes model row numbers. Use sourceRow() or at() before mutating the source from a
 * selection in the filtered view.
 *
 * @code
 * ObservableVector<Person> people;
 * auto model = ObservableListModel<Person>::create(
 *     people, []( const Person& person, ModelRole role ) {
 *         return role == ModelRole::Display ? Variant( person.name ) : Variant{};
 *     } );
 * model->setFilter( []( const Person& person ) { return person.active; } );
 * @endcode
 */
template <typename T> class ObservableListModel final : public Model {
  public:
	/** @brief Converts one source item and role into model data. */
	using Formatter = std::function<Variant( const T&, ModelRole )>;

	/** @brief Returns true when a source item belongs in the visible projection. */
	using Predicate = std::function<bool( const T& )>;

	/** @brief Creates a one-column adapter using Variant's standard conversion for Display data. */
	static std::shared_ptr<ObservableListModel> create( ObservableVector<T>& source ) {
		return std::make_shared<ObservableListModel>( source );
	}

	/** @brief Creates a one-column adapter whose data() is supplied by @p formatter. */
	static std::shared_ptr<ObservableListModel> create( ObservableVector<T>& source,
														Formatter formatter ) {
		return std::make_shared<ObservableListModel>( source, std::move( formatter ) );
	}

	/**
	 * @brief Creates an adapter over @p source, optionally using @p formatter for all roles.
	 *
	 * The source storage is retained for the model's lifetime.
	 */
	explicit ObservableListModel( ObservableVector<T>& source, Formatter formatter = {} ) :
		mSource( source.sharedHandle() ), mFormatter( std::move( formatter ) ) {
		mConnection = mSource.observe(
			[this]( const typename ObservableVector<T>::Change& change ) { onChange( change ); } );
	}
	~ObservableListModel() { mConnection.disconnect(); }

	/** @return The number of visible rows in the current projection. */
	size_t rowCount( const ModelIndex& = ModelIndex() ) const {
		return mPredicate ? mRows.size() : mSource.size();
	}

	/** @return One; ObservableListModel is a flat, one-column model. */
	size_t columnCount( const ModelIndex& = ModelIndex() ) const { return 1; }

	/** @return A valid index for a visible row in column zero, or an invalid index. */
	ModelIndex index( int row, int column = 0, const ModelIndex& parent = ModelIndex() ) const {
		if ( row < 0 || column != 0 || static_cast<std::size_t>( row ) >= rowCount( parent ) )
			return {};
		return Model::index( row, column, parent );
	}

	/** @return Formatted data for @p index and @p role, or an empty Variant when unavailable. */
	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		const T* value = at( index );
		if ( !value )
			return {};
		if ( mFormatter )
			return mFormatter( *value, role );
		if ( role != ModelRole::Display )
			return {};
		if constexpr ( std::is_constructible_v<Variant, const T&> )
			return Variant( *value );
		return {};
	}

	/**
	 * @brief Replaces the visible filter and invalidates all model indexes.
	 *
	 * An empty predicate is equivalent to no filter. While filtered, source mutations rebuild the
	 * projection and invalidate indexes instead of emitting incremental row notifications.
	 */
	void setFilter( Predicate predicate ) {
		mPredicate = std::move( predicate );
		rebuildRows();
		invalidate( InvalidateAllIndexes );
	}

	/** @brief Removes the active filter and exposes all source rows. */
	void clearFilter() {
		if ( !mPredicate )
			return;
		mPredicate = {};
		mRows.clear();
		invalidate( InvalidateAllIndexes );
	}

	/**
	 * @return The ObservableVector row represented by @p index, or std::nullopt for an invalid or
	 * foreign index.
	 */
	std::optional<std::size_t> sourceRow( const ModelIndex& index ) const {
		if ( !index.isValid() || index.model() != this || index.row() < 0 ||
			 static_cast<std::size_t>( index.row() ) >= rowCount() )
			return {};
		return mPredicate ? mRows[index.row()] : static_cast<std::size_t>( index.row() );
	}

	/**
	 * @return The source item represented by @p index, or nullptr for an invalid or foreign index.
	 * @warning The pointer is invalidated by mutations that reallocate or remove vector elements.
	 */
	const T* at( const ModelIndex& index ) const {
		auto row = sourceRow( index );
		return row ? &mSource[*row] : nullptr;
	}

  private:
	void rebuildRows() {
		mRows.clear();
		if ( !mPredicate )
			return;
		for ( std::size_t i = 0; i < mSource.size(); ++i )
			if ( mPredicate( mSource[i] ) )
				mRows.push_back( i );
	}
	void onChange( const typename ObservableVector<T>::Change& change ) {
		using ChangeType = typename ObservableVector<T>::ChangeType;
		using Phase = typename ObservableVector<T>::Phase;
		if ( mPredicate ) {
			if ( change.phase == Phase::After ) {
				rebuildRows();
				invalidate( InvalidateAllIndexes );
			}
			return;
		}
		const int first = static_cast<int>( change.index );
		const int last = static_cast<int>( change.index + change.count - 1 );
		if ( change.phase == Phase::Before ) {
			if ( change.type == ChangeType::Insert )
				beginInsertRows( {}, first, last );
			else if ( change.type == ChangeType::Remove )
				beginDeleteRows( {}, first, last );
			else if ( change.type == ChangeType::Move )
				beginMoveRows( {}, first, last, {}, static_cast<int>( change.target ) );
		} else {
			if ( change.type == ChangeType::Insert )
				endInsertRows();
			else if ( change.type == ChangeType::Remove )
				endDeleteRows();
			else if ( change.type == ChangeType::Move )
				endMoveRows();
			else if ( change.type == ChangeType::Change )
				invalidate( DontInvalidateIndexes );
			else if ( change.type == ChangeType::Reset )
				invalidate( InvalidateAllIndexes );
		}
	}
	typename ObservableVector<T>::SharedHandle mSource;
	Formatter mFormatter;
	Predicate mPredicate;
	std::vector<std::size_t> mRows;
	typename ObservableVector<T>::Connection mConnection;
};

}}} // namespace EE::UI::Models

#endif
