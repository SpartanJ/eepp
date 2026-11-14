#include "pane.hpp"
#include <algorithm>

namespace eefm {

void FileTab::selectLater( SmallVector<std::string, 1> names ) {
	std::sort( names.begin(), names.end() );
	selectWhenListed = std::move( names );
	selectWhenListedSince.restart();
}

void FileTab::invertSelection() {
	const auto selected = selectedRows();
	const size_t count = model->rowCount();
	std::vector<ModelIndex> inverted;
	inverted.reserve( count - selected.size() );
	auto next = selected.begin();
	for ( size_t row = 0; row < count; ++row ) {
		if ( next != selected.end() && *next == row )
			++next;
		else
			inverted.push_back( model->index( row ) );
	}
	view->getSelection().set( inverted );
}

std::vector<size_t> FileTab::selectedRows() const {
	const auto indexes = view->getSelection().indexes();
	std::vector<size_t> rows;
	rows.reserve( indexes.size() );
	for ( const auto& index : indexes ) {
		if ( index.row() >= 0 && static_cast<size_t>( index.row() ) < model->rowCount() )
			rows.push_back( static_cast<size_t>( index.row() ) );
	}
	std::sort( rows.begin(), rows.end() );
	rows.erase( std::unique( rows.begin(), rows.end() ), rows.end() );
	return rows;
}

std::vector<URI> FileTab::selectedURIs() const {
	const auto rows = selectedRows();
	std::vector<URI> uris;
	uris.reserve( rows.size() );
	for ( size_t row : rows )
		uris.push_back( model->entryURI( row ) );
	return uris;
}

} // namespace eefm
