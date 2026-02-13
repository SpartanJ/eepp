#include "llmprovidermodel.hpp"
#include <eepp/core/string.hpp>

namespace ecode {

std::shared_ptr<LLMProviderModel> LLMProviderModel::create( const LLMProviders& model ) {
	return std::make_shared<LLMProviderModel>( model );
}

LLMProviderModel::LLMProviderModel( const LLMProviders& model ) : mProviders( model ) {
	resetFilter();
}

LLMProviderModel::~LLMProviderModel() {}

size_t LLMProviderModel::rowCount( const ModelIndex& parent ) const {
	if ( !parent.isValid() )
		return mItems.size();

	if ( parent.internalId() == -1 )
		return mItems[parent.row()].models.size();

	return 0;
}

size_t LLMProviderModel::columnCount( const ModelIndex& ) const {
	return 1;
}

ModelIndex LLMProviderModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 )
		return {};

	if ( !parent.isValid() ) {
		if ( row < (int)mItems.size() )
			return createIndex( row, column, (void*)mItems[row].provider, -1 );
		return {};
	}

	if ( parent.internalId() == -1 ) {
		// Parent is a provider
		if ( row < (int)mItems[parent.row()].models.size() )
			return createIndex( row, column, (void*)mItems[parent.row()].models[row],
								parent.row() );
	}

	return {};
}

ModelIndex LLMProviderModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() || index.internalId() == -1 )
		return {};

	// internalId is the row index of the provider
	int providerRow = (int)index.internalId();
	if ( providerRow >= 0 && providerRow < (int)mItems.size() ) {
		return createIndex( providerRow, 0, (void*)mItems[providerRow].provider, -1 );
	}

	return {};
}

Variant LLMProviderModel::data( const ModelIndex& index, ModelRole role ) const {
	if ( !index.isValid() )
		return {};

	if ( role == ModelRole::Display ) {
		if ( index.internalId() == -1 ) {
			const LLMProvider* provider = static_cast<const LLMProvider*>( index.internalData() );
			return Variant( provider->displayName.value_or( provider->name ) );
		} else {
			const LLMModel* model = static_cast<const LLMModel*>( index.internalData() );
			return Variant( model->displayName.value_or( model->name ) );
		}
	} else if ( role == ModelRole::Custom ) {
		return Variant( index.internalData() );
	}

	return {};
}

void LLMProviderModel::filter( const std::string& filterText ) {
	if ( filterText.empty() ) {
		resetFilter();
		return;
	}

	mItems.clear();
	std::string lowerFilter = String::toLower( filterText );

	for ( const auto& [key, provider] : mProviders ) {
		std::string pName = provider.displayName.value_or( provider.name );
		bool providerMatches = String::icontains( pName, lowerFilter );

		ProviderData data;
		data.provider = &provider;

		if ( providerMatches ) {
			for ( const auto& model : provider.models ) {
				data.models.push_back( &model );
			}
		} else {
			for ( const auto& model : provider.models ) {
				std::string mName = model.displayName.value_or( model.name );
				if ( String::icontains( mName, lowerFilter ) ) {
					data.models.push_back( &model );
				}
			}
		}

		if ( providerMatches || !data.models.empty() ) {
			mItems.emplace_back( std::move( data ) );
		}
	}
	invalidate( Model::UpdateFlag::InvalidateAllIndexes );
}

void LLMProviderModel::resetFilter() {
	mItems.clear();
	for ( const auto& [key, provider] : mProviders ) {
		ProviderData data;
		data.provider = &provider;
		for ( const auto& model : provider.models ) {
			data.models.push_back( &model );
		}
		mItems.emplace_back( std::move( data ) );
	}
	invalidate( Model::UpdateFlag::InvalidateAllIndexes );
}

} // namespace ecode
