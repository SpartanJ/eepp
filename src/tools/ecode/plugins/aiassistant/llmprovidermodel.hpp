#ifndef ECODE_AIASSISTANT_LLMPROVIDERMODEL_HPP
#define ECODE_AIASSISTANT_LLMPROVIDERMODEL_HPP

#include "protocol.hpp"
#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI::Models;

namespace ecode {

class LLMProviderModel : public Model {
  public:
	static std::shared_ptr<LLMProviderModel> create( const LLMProviders& model );

	explicit LLMProviderModel( const LLMProviders& model );

	virtual ~LLMProviderModel();

	virtual size_t rowCount( const ModelIndex& parent = ModelIndex() ) const override;

	virtual size_t columnCount( const ModelIndex& parent = ModelIndex() ) const override;

	virtual ModelIndex index( int row, int column,
							  const ModelIndex& parent = ModelIndex() ) const override;

	virtual ModelIndex parentIndex( const ModelIndex& index ) const override;

	virtual Variant data( const ModelIndex& index,
						  ModelRole role = ModelRole::Display ) const override;

	void filter( const std::string& filter );

  protected:
	struct ProviderData {
		const LLMProvider* provider;
		std::vector<const LLMModel*> models;
	};

	LLMProviders mProviders;
	std::vector<ProviderData> mItems;

	void resetFilter();
};

} // namespace ecode

#endif // ECODE_AIASSISTANT_LLMPROVIDERMODEL_HPP
