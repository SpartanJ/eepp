#ifndef EE_UI_MODELS_STRINGMAPMODEL_HPP
#define EE_UI_MODELS_STRINGMAPMODEL_HPP

#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/ui/models/model.hpp>
#include <map>
#include <string>
#include <vector>
#include <deque>

namespace EE { namespace UI { namespace Models {

template <typename StringType = std::string> class StringMapModel : public Model {
  public:
	struct Node {
		const StringType* text;
		Node* parent = nullptr;
		std::vector<Node*> children;
		std::vector<Node*> visibleChildren;

		Node( const StringType* text, Node* parent ) : text( text ), parent( parent ) {}

		size_t childCount() const { return visibleChildren.size(); }
	};

	static std::shared_ptr<StringMapModel<StringType>>
	create( const std::map<StringType, std::vector<StringType>>& map ) {
		return std::make_shared<StringMapModel<StringType>>( map );
	}

	explicit StringMapModel( const std::map<StringType, std::vector<StringType>>& map ) : mMap( map ) {
		mNodes.emplace_back( &mEmptyString, nullptr );
		mRoot = &mNodes.back();

		for ( const auto& [category, items] : mMap ) {
			mNodes.emplace_back( &category, mRoot );
			Node* catNode = &mNodes.back();
			mRoot->children.push_back( catNode );

			for ( const auto& item : items ) {
				mNodes.emplace_back( &item, catNode );
				catNode->children.push_back( &mNodes.back() );
			}
		}
		resetFilter();
	}

	virtual ~StringMapModel() {}

	virtual size_t rowCount( const ModelIndex& parent = ModelIndex() ) const override {
		if ( !parent.isValid() )
			return mRoot->visibleChildren.size();
		Node* node = static_cast<Node*>( parent.internalData() );
		return node->visibleChildren.size();
	}

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const override { return 1; }

	virtual ModelIndex index( int row, int column,
							  const ModelIndex& parent = ModelIndex() ) const override {
		if ( row < 0 || column < 0 || row >= (int)rowCount( parent ) ||
			 column >= (int)columnCount( parent ) )
			return {};
		Node* parentNode =
			parent.isValid() ? static_cast<Node*>( parent.internalData() ) : mRoot;
		if ( row < (int)parentNode->visibleChildren.size() )
			return createIndex( row, column, parentNode->visibleChildren[row] );
		return {};
	}

	virtual ModelIndex parent( const ModelIndex& index ) const {
		if ( !index.isValid() )
			return {};
		Node* node = static_cast<Node*>( index.internalData() );
		if ( node->parent == mRoot || node->parent == nullptr )
			return {};
		Node* grandParent = node->parent->parent;
		if ( !grandParent )
			return {};
		// Find row of parent in grandparent's visible children
		auto it = std::find( grandParent->visibleChildren.begin(),
							 grandParent->visibleChildren.end(), node->parent );
		if ( it != grandParent->visibleChildren.end() ) {
			int row = std::distance( grandParent->visibleChildren.begin(), it );
			return createIndex( row, 0, node->parent );
		}
		return {};
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		if ( !index.isValid() )
			return {};
		if ( role == ModelRole::Display ) {
			Node* node = static_cast<Node*>( index.internalData() );
			return Variant( *node->text );
		}
		return {};
	}

	void filter( const std::string_view& filterText ) {
		if ( filterText.empty() ) {
			resetFilter();
			return;
		}
		mRoot->visibleChildren.clear();
		for ( const auto& cat : mRoot->children ) {
			bool catMatches = String::icontains( *cat->text, filterText );
			cat->visibleChildren.clear();
			for ( const auto& item : cat->children ) {
				if ( catMatches || String::icontains( *item->text, filterText ) ) {
					cat->visibleChildren.push_back( item );
				}
			}

			if ( catMatches || !cat->visibleChildren.empty() ) {
				mRoot->visibleChildren.push_back( cat );
			}
		}
		invalidate( Model::UpdateFlag::InvalidateAllIndexes );
	}

	void resetFilter() {
		mRoot->visibleChildren.clear();
		for ( const auto& cat : mRoot->children ) {
			cat->visibleChildren.clear();
			for ( const auto& item : cat->children ) {
				cat->visibleChildren.push_back( item );
			}
			mRoot->visibleChildren.push_back( cat );
		}
		invalidate( Model::UpdateFlag::InvalidateAllIndexes );
	}

  private:
	std::map<StringType, std::vector<StringType>> mMap;
	StringType mEmptyString;
	std::deque<Node> mNodes;
	Node* mRoot{ nullptr };
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_STRINGMAPMODEL_HPP
