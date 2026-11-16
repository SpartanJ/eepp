#ifndef EE_UI_MODELS_CSSPROPERTIESMODEL_HPP
#define EE_UI_MODELS_CSSPROPERTIESMODEL_HPP

#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE::UI::Models {

/** Snapshot of native computed values and their CSS origins, for developer inspection. */
class EE_API CSSPropertiesModel final : public Model {
  public:
	static std::shared_ptr<CSSPropertiesModel> create( UIWidget* widget = nullptr );

	explicit CSSPropertiesModel( UIWidget* widget = nullptr );

	virtual ~CSSPropertiesModel();

	virtual size_t rowCount( const ModelIndex& = {} ) const;

	virtual size_t columnCount( const ModelIndex& = {} ) const;

	virtual std::string columnName( const size_t& column ) const;

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const;

	virtual void update();

	void setWidget( UIWidget* widget );

	UIWidget* getWidget() const { return mWidget; }

	/** Refreshes values and origins; returns true when the Style text changed. */
	bool refresh();

	const std::string& getStyleText() const { return mStyleText; }

	/** Borrowed ancestor for a source row. Refresh before using, and validate tree membership. */
	UIWidget* sourceWidgetAt( Int64 row ) const;

  private:
	struct Row {
		const CSS::PropertyDefinition* definition;
		UIWidget* sourceWidget;
		std::string value;
		std::string source;

		bool operator==( const Row& ) const = default;
	};

	UIWidget* mWidget{ nullptr };
	std::vector<Row> mRows;
	std::vector<Row> mScratchRows;
	std::string mStyleText;
	std::string mScratchStyleText;
	Uint32 mCloseCb{ 0 };

	static void appendWidgetName( std::string& output, const UIWidget* widget );

	void buildStyleText();
};

} // namespace EE::UI::Models

#endif // EE_UI_MODELS_CSSPROPERTIESMODEL_HPP
