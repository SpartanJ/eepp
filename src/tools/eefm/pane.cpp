#include "pane.hpp"
#include <algorithm>

namespace eefm {

std::vector<size_t> FileTab::selectedRows() const {
	std::vector<size_t> rows;
	for ( const auto& index : view->getSelection().indexes() ) {
		if ( index.row() >= 0 && static_cast<size_t>( index.row() ) < model->rowCount() &&
			 std::find( rows.begin(), rows.end(), static_cast<size_t>( index.row() ) ) ==
				 rows.end() ) {
			rows.push_back( static_cast<size_t>( index.row() ) );
		}
	}
	std::sort( rows.begin(), rows.end() );
	return rows;
}

std::vector<URI> FileTab::selectedURIs() const {
	std::vector<URI> uris;
	for ( size_t row : selectedRows() )
		uris.push_back( model->entryURI( row ) );
	return uris;
}

} // namespace eefm
