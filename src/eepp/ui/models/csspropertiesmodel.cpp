#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/models/csspropertiesmodel.hpp>
#include <eepp/ui/uistyle.hpp>

#include <algorithm>

namespace EE::UI::Models {

std::shared_ptr<CSSPropertiesModel> CSSPropertiesModel::create( UIWidget* widget ) {
	return std::make_shared<CSSPropertiesModel>( widget );
}

CSSPropertiesModel::CSSPropertiesModel( UIWidget* widget ) {
	setWidget( widget );
}

CSSPropertiesModel::~CSSPropertiesModel() {
	if ( mWidget && mCloseCb )
		mWidget->removeEventListener( mCloseCb );
}

size_t CSSPropertiesModel::rowCount( const ModelIndex& ) const {
	return mRows.size();
}

size_t CSSPropertiesModel::columnCount( const ModelIndex& ) const {
	return 3;
}

std::string CSSPropertiesModel::columnName( const size_t& column ) const {
	return column == 0 ? "Property" : column == 1 ? "Value" : "Source";
}

Variant CSSPropertiesModel::data( const ModelIndex& index, ModelRole role ) const {
	if ( !index.isValid() || index.row() < 0 || index.row() >= static_cast<Int64>( mRows.size() ) ||
		 index.column() < 0 || index.column() >= 3 )
		return {};
	const auto& row = mRows[index.row()];
	if ( role == ModelRole::Tooltip ) {
		if ( index.column() == 2 && row.sourceWidget && row.sourceWidget != mWidget ) {
			return Variant( row.source +
							"\nClick to select the ancestor supplying this property." );
		}
		return Variant::fromRef( index.column() == 0   ? row.definition->getName()
								 : index.column() == 1 ? row.value
													   : row.source );
	}
	if ( role != ModelRole::Display && role != ModelRole::Sort )
		return {};
	if ( index.column() == 0 )
		return Variant::fromRef( row.definition->getName() );
	return Variant::fromRef( index.column() == 1 ? row.value : row.source );
}

void CSSPropertiesModel::setWidget( UIWidget* widget ) {
	if ( mWidget && mCloseCb )
		mWidget->removeEventListener( mCloseCb );
	mWidget = widget;
	mCloseCb = 0;
	if ( mWidget ) {
		mCloseCb = mWidget->on( Event::OnClose, [this]( const Event* ) {
			mWidget = nullptr;
			mCloseCb = 0;
			refresh();
		} );
	}
	refresh();
}

void CSSPropertiesModel::appendWidgetName( std::string& output, const UIWidget* widget ) {
	output += widget->getElementTag();
	if ( !widget->getId().empty() ) {
		output += '#';
		output += widget->getId();
	}
}

bool CSSPropertiesModel::refresh() {
	size_t rowCount = 0;
	CSS::PropertyIdSet seen;
	if ( mWidget ) {
		auto properties = mWidget->getPropertiesImplemented();
		mScratchRows.reserve( properties.size() );
		auto* style = mWidget->getUIStyle();
		for ( auto id : properties ) {
			const auto* definition = CSS::StyleSheetSpecification::instance()->getProperty( id );
			if ( !definition || seen.contains( id ) )
				continue;
			seen.insert( id );
			auto source = style ? style->getPropertySource( id ) : UIStyle::PropertySource{};
			if ( rowCount == mScratchRows.size() )
				mScratchRows.emplace_back();
			auto& row = mScratchRows[rowCount++];
			row.definition = definition;
			row.sourceWidget = source.widget;
			row.value = mWidget->getPropertyString( definition );
			row.source.clear();
			if ( !source.property ) {
				row.source = "Native / default";
			} else {
				if ( source.widget != mWidget ) {
					row.source = "Inherited from ";
					appendWidgetName( row.source, source.widget );
					row.source += " (";
				}
				if ( source.rule )
					row.source += source.rule->getSelector().getName();
				else
					row.source += "Element style";
				if ( source.widget != mWidget )
					row.source += ')';
			}
		}
	}
	mScratchRows.resize( rowCount );
	if ( mRows != mScratchRows ) {
		const bool sameProperties =
			mRows.size() == mScratchRows.size() &&
			std::equal( mRows.begin(), mRows.end(), mScratchRows.begin(),
						[]( const Row& a, const Row& b ) { return a.definition == b.definition; } );
		mRows.swap( mScratchRows );
		invalidate( sameProperties ? UpdateFlag::DontInvalidateIndexes
								   : UpdateFlag::InvalidateAllIndexes );
	}
	buildStyleText();
	if ( mStyleText == mScratchStyleText )
		return false;
	mStyleText.swap( mScratchStyleText );
	return true;
}

void CSSPropertiesModel::update() {
	refresh();
}

UIWidget* CSSPropertiesModel::sourceWidgetAt( Int64 row ) const {
	return row >= 0 && row < static_cast<Int64>( mRows.size() ) ? mRows[row].sourceWidget : nullptr;
}

void CSSPropertiesModel::buildStyleText() {
	mScratchStyleText.clear();
	if ( !mWidget )
		return;
	auto* selectedStyle = mWidget->getUIStyle();
	if ( !selectedStyle )
		return;
	// Gather ancestor declarations only while inspecting. The ordinary style/render paths do not
	// store provenance, copies of rule text, or subscriptions for the inspector.
	for ( UIWidget* widget = mWidget; widget; ) {
		auto* style = widget->getUIStyle();
		const bool inherited = widget != mWidget;
		const auto sectionStart = mScratchStyleText.size();
		if ( inherited ) {
			mScratchStyleText += "\n/* Inherited from ";
			appendWidgetName( mScratchStyleText, widget );
			mScratchStyleText += " */\n";
		}
		const auto headerEnd = mScratchStyleText.size();
		auto appendRule = [&]( const CSS::StyleSheetStyle* rule, bool element ) {
			// Native CSS indexes :root as a global wildcard for variables. Display its rules
			// once at the actual root, rather than repeating them on every inspected ancestor.
			if ( !element && rule->getSelector().getName() == ":root" &&
				 widget->getElementTag() != ":root" )
				return;
			const auto start = mScratchStyleText.size();
			mScratchStyleText += element ? "/* Element style */\n" : "";
			mScratchStyleText += element ? widget->getElementTag() : rule->getSelector().getName();
			mScratchStyleText += " {\n";
			const auto bodyStart = mScratchStyleText.size();
			for ( const auto& [key, property] : rule->getProperties() ) {
				const auto* definition = property.getPropertyDefinition();
				if ( inherited ) {
					if ( !definition )
						continue;
					const auto source =
						selectedStyle->getPropertySource( definition->getPropertyId() );
					if ( !definition->isInherited() &&
						 ( !source.widget || source.widget == mWidget ) )
						continue;
				}
				mScratchStyleText += '\t';
				mScratchStyleText += property.getName();
				mScratchStyleText += ": ";
				mScratchStyleText += property.value();
				mScratchStyleText += ';';
				if ( definition ) {
					const auto id = definition->getPropertyId();
					const auto source = selectedStyle->getPropertySource( id );
					const auto* local = style->getCascadedProperty( id );
					// An explicit inherit declaration is active even though its value comes from
					// an ancestor. Compare rule identity/order, never substituted value strings.
					const auto ownSource = style->getPropertySource( id );
					bool onInheritancePath = false;
					for ( Node* node = mWidget; node && node != source.widget;
						  node = node->getParent() ) {
						if ( node == widget ) {
							onInheritancePath = true;
							break;
						}
					}
					const bool explicitInherit =
						onInheritancePath && local && ownSource.property == source.property &&
						local->getSpecificity() == property.getSpecificity() &&
						local->getDeclarationOrder() == property.getDeclarationOrder();
					const bool active =
						( source.widget == widget &&
						  ( element ? source.rule == nullptr : source.rule == rule ) ) ||
						explicitInherit;
					if ( !active )
						mScratchStyleText += " /* overridden */";
				}
				mScratchStyleText += '\n';
			}
			for ( const auto& [key, variable] : rule->getVariables() ) {
				mScratchStyleText += '\t';
				mScratchStyleText += variable.getName();
				mScratchStyleText += ": ";
				mScratchStyleText += variable.getValue();
				mScratchStyleText += ";\n";
			}
			if ( mScratchStyleText.size() == bodyStart )
				mScratchStyleText.resize( start );
			else
				mScratchStyleText += "}\n\n";
		};
		if ( style ) {
			if ( auto definition = style->getDefinition() ) {
				for ( auto* rule : definition->getStyles() )
					appendRule( rule, false );
			}
			if ( const auto* element = style->getElementStyle() )
				appendRule( element, true );
		}
		if ( mScratchStyleText.size() == headerEnd )
			mScratchStyleText.resize( sectionStart );
		auto* parent = widget->getParent();
		widget = parent && parent->isWidget() ? parent->asType<UIWidget>() : nullptr;
	}
}

} // namespace EE::UI::Models
