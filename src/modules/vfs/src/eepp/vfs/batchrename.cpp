#include "batchrenameengine.hpp"
#include <charconv>
#include <eepp/core/containers.hpp>
#include <eepp/core/string.hpp>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <limits>

namespace EE { namespace VFS {

VFSResult<std::string> BatchRenamePattern::apply( const BatchRenameItem& item,
												  Uint64 offset ) const {
	auto invalid = []( const char* message ) -> VFSResult<std::string> {
		return Err( VFSError( VFSErrorCode::InvalidOperation, {}, message ) );
	};
	if ( text.empty() )
		return invalid( "The rename text cannot be empty" );
	std::string_view stem( item.name );
	std::string_view suffix;
	if ( !item.extension.empty() ) {
		if ( stem.size() <= item.extension.size() ||
			 stem[stem.size() - item.extension.size() - 1] != '.' ||
			 stem.substr( stem.size() - item.extension.size() ) != item.extension ) {
			return invalid( "The extension does not match the original name" );
		}
		suffix = stem.substr( stem.size() - item.extension.size() - 1 );
		stem.remove_suffix( suffix.size() );
	}
	std::string output;
	if ( mode == Mode::Enumerate ) {
		if ( position != Position::Replace && position != Position::Before &&
			 position != Position::After )
			return invalid( "Unknown numbering position" );
		const size_t start = text.find( '#' );
		size_t end = start;
		if ( start != std::string::npos ) {
			while ( end < text.size() && text[end] == '#' )
				++end;
		}
		const bool numbered =
			start != std::string::npos && text.find( '#', end ) == std::string::npos;
		if ( !numbered && position == Position::Replace && !allowUnnumbered )
			return invalid( "The new name must contain one consecutive sequence of #" );
		output = text;
		if ( numbered ) {
			if ( offset > std::numeric_limits<Uint64>::max() - startIndex )
				return invalid( "The rename index is too large" );
			char buffer[20];
			const auto number =
				std::to_chars( buffer, buffer + sizeof( buffer ), startIndex + offset );
			const size_t digits = number.ptr - buffer;
			output.replace( start, end - start, end - start > digits ? end - start - digits : 0,
							'0' );
			output.insert( start + ( end - start > digits ? end - start - digits : 0 ), buffer,
						   digits );
		}
		if ( position == Position::Before )
			output.append( stem );
		else if ( position == Position::After )
			output.insert( 0, stem );
	} else if ( mode == Mode::ReplaceText ) {
		output.reserve( stem.size() );
		size_t cursor = 0;
		for ( size_t found = stem.find( text ); found != std::string_view::npos;
			  found = stem.find( text, cursor ) ) {
			output.append( stem.substr( cursor, found - cursor ) );
			output.append( replacement );
			cursor = found + text.size();
		}
		output.append( stem.substr( cursor ) );
		const size_t first = output.find_first_not_of( ' ' );
		output.erase( 0, first == std::string::npos ? output.size() : first );
	} else if ( mode == Mode::AddText ) {
		if ( position != Position::Before && position != Position::After )
			return invalid( "Choose whether to add text before or after the filename" );
		output.reserve( stem.size() + text.size() + suffix.size() );
		if ( position == Position::Before ) {
			output = text;
			output.append( stem );
		} else {
			output = stem;
			output.append( text );
		}
	} else {
		return invalid( "Unknown rename mode" );
	}
	if ( output.empty() )
		return invalid( "A filename cannot be empty" );
	if ( !suffix.empty() &&
		 ( output.size() < suffix.size() ||
		   output.compare( output.size() - suffix.size(), suffix.size(), suffix ) != 0 ) ) {
		output.append( suffix );
	}
	return Ok( std::move( output ) );
}

BatchRenameResult BatchRenameEngine::run( VirtualFileSystem& vfs, const URI& directory,
										  std::vector<BatchRenameItem> items,
										  const BatchRenamePattern& pattern,
										  CancellationToken token ) {
	BatchRenameResult result;
	result.items = std::move( items );
	if ( result.items.empty() )
		return result;
	if ( !VFSPath::validURI( directory ) ) {
		result.error = VFSError( VFSErrorCode::InvalidURI, directory );
		return result;
	}
	const auto caps = vfs.capabilities( directory );
	if ( !caps.has( VFSCapability::Rename ) ) {
		result.error =
			VFSError( VFSErrorCode::Unsupported, directory, "This location cannot rename items" );
		return result;
	}
	result.names.reserve( result.items.size() );
	const bool caseSensitive = caps.has( VFSCapability::CaseSensitive );
	// Case-sensitive providers borrow the owned names. Other providers need folded keys only
	// during planning; reserve their storage so the hash table's views remain stable.
	std::vector<std::string> foldedSources, foldedTargets;
	if ( !caseSensitive ) {
		foldedSources.reserve( result.items.size() );
		foldedTargets.reserve( result.items.size() );
	}
	UnorderedSet<std::string_view> sources;
	UnorderedSet<std::string_view> targets;
	sources.reserve( result.items.size() );
	targets.reserve( result.items.size() );
	for ( size_t i = 0; i < result.items.size(); ++i ) {
		const auto& item = result.items[i];
		if ( token.isCancellationRequested() ) {
			result.error = cancelledError( directory );
			return result;
		}
		auto name = pattern.apply( item, i );
		if ( !name ) {
			result.error = std::move( name ).error();
			return result;
		}
		if ( !VFSPath::validName( item.name, directory ) ||
			 !VFSPath::validName( name.value(), directory ) ) {
			result.error = VFSError( VFSErrorCode::InvalidOperation, directory,
									 "A batch entry has an invalid filename" );
			return result;
		}
		result.names.push_back( std::move( name ).value() );
		if ( !caseSensitive ) {
			foldedSources.push_back( String::toLower( item.name ) );
			foldedTargets.push_back( String::toLower( result.names.back() ) );
		}
		if ( !sources.insert( caseSensitive ? item.name : foldedSources.back() ).second ||
			 !targets.insert( caseSensitive ? result.names.back() : foldedTargets.back() )
				  .second ) {
			result.error = VFSError( VFSErrorCode::AlreadyExists, directory,
									 "The batch contains duplicate source or destination names" );
			return result;
		}
	}
	// Validate the whole batch before the first mutation. Provider no-overwrite renames still
	// enforce collisions at execution time if another writer changes the directory afterwards.
	for ( size_t i = 0; i < result.items.size(); ++i ) {
		if ( token.isCancellationRequested() ) {
			result.error = cancelledError( directory );
			return result;
		}
		const URI source = directory.withAppendedSegment( result.items[i].name );
		auto info = detail::await( vfs.stat( source, StatOptions{ false }, token ) );
		if ( !info ) {
			result.error = std::move( info ).error();
			return result;
		}
		if ( result.items[i].name == result.names[i] )
			continue;
		const URI target = directory.withAppendedSegment( result.names[i] );
		auto existing = detail::await( vfs.stat( target, StatOptions{ false }, token ) );
		if ( existing && !caseSensitive &&
			 String::icompare( result.items[i].name, result.names[i] ) == 0 &&
			 info.value().identity() && existing.value().identity() == info.value().identity() ) {
			continue; // A case-only rename of the same object is decided by the provider.
		}
		if ( existing || !existing.error().is( VFSErrorCode::NotFound ) ) {
			result.error = existing ? VFSError( VFSErrorCode::AlreadyExists, target,
												"A destination already exists" )
									: std::move( existing ).error();
			return result;
		}
	}
	for ( size_t i = 0; i < result.items.size(); ++i ) {
		if ( token.isCancellationRequested() ) {
			result.error = cancelledError( directory );
			return result;
		}
		if ( result.items[i].name != result.names[i] ) {
			auto renamed = detail::await(
				vfs.rename( directory.withAppendedSegment( result.items[i].name ),
							directory.withAppendedSegment( result.names[i] ), {}, token ) );
			if ( !renamed ) {
				result.error = std::move( renamed ).error();
				return result;
			}
		}
		++result.completed;
	}
	return result;
}

}} // namespace EE::VFS
