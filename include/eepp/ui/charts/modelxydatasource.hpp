#ifndef EE_UI_CHARTS_MODELXYDATASOURCE_HPP
#define EE_UI_CHARTS_MODELXYDATASOURCE_HPP

#include <eepp/ui/charts/xydatasource.hpp>
#include <eepp/ui/models/model.hpp>

namespace EE { namespace UI { namespace Charts {

/** Materializes model numeric roles into a typed cache when the model changes. */
class EE_API ModelXYDataSource final : public XYDataSource, public Models::Model::Client {
  public:
	ModelXYDataSource( std::shared_ptr<Models::Model> model, size_t xColumn, size_t yColumn,
					   Models::ModelRole xRole = Models::ModelRole::Display,
					   Models::ModelRole yRole = Models::ModelRole::Display );

	~ModelXYDataSource() override;

	XYDataRead acquireRead() const override { return mCache.acquireRead(); }

	void refresh();

	void onModelUpdated( unsigned flags ) override;

  private:
	std::shared_ptr<Models::Model> mModel;
	OwnedXYDataSource mCache;
	size_t mXColumn;
	size_t mYColumn;
	Models::ModelRole mXRole;
	Models::ModelRole mYRole;
};

}}} // namespace EE::UI::Charts

#endif
