#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYTEXTINDEX_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYTEXTINDEX_HPP

#include <algorithm>
#include <bit>
#include <eepp/core/small_vector.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <limits>

namespace EE { namespace UI {

class UIWidget;

/** Optional, UI-thread-owned code-point index. Document lines include their newline; the last
 * newline is a sentinel and is not exposed as text. No document pointer or observer is retained.
 * A Fenwick prefix-sum tree over 256-line blocks avoids the measured linear cost of scanning
 * block totals for warm queries near EOF. Single-line edits read one document line and update
 * O(log(blocks)) totals. Structural edits shift the flat lengths and rebuild totals, but read
 * only the affected document lines. */
class AccessibilityTextIndex {
  public:
	static constexpr size_t BlockSize = 256;

	void ensure( const Doc::TextDocument& document ) {
		if ( mValid && mDocument == document.getUUID() &&
			 mRevision == document.getModificationId() )
			return;
		mDocument = document.getUUID();
		// Cold initialization scans every line; acquire the document locks once, not per line.
		mRevision = document.getLineLengths( mLengths );
		mLineReads += mLengths.size();
		++mRebuilds;
		rebuildBlocks();
		mValid = true;
	}

	/** Notifications describe pre-edit ranges but arrive after mutation. The revision alone is
	 * not a commit marker: a query in an earlier callback may already have read this revision.
	 * Validate its affected lengths instead of applying a structural delta twice. Missed/nested
	 * revisions and whole-text changes invalidate lazily; they never guess at an old edit shape.
	 * Returns false for ambiguous/missed deltas: native events must use a whole-text replacement
	 * rather than forwarding edits that may have reached callbacks in a different order. */
	bool changed( const Doc::TextDocument& document, const Doc::DocumentContentChange* change ) {
		if ( !mValid )
			return !mAmbiguousEdits;
		if ( !change || mDocument != document.getUUID() ) {
			mValid = false;
			return change ? invalidateAmbiguous() : !mAmbiguousEdits;
		}
		const auto range = change->range.normalized();
		const size_t count = document.linesCount();
		if ( !range.start().isValid() || !range.end().isValid() || change->text.size() > 65536 ) {
			mValid = false;
			// A large insertion has a trustworthy delta, but is cheaper to index lazily.
			if ( !range.start().isValid() || !range.end().isValid() )
				return invalidateAmbiguous();
			return !mAmbiguousEdits;
		}
		const size_t first = static_cast<size_t>( range.start().line() );
		const size_t last = static_cast<size_t>( range.end().line() );
		const size_t replacement =
			1 + static_cast<size_t>( std::count( change->text.begin(), change->text.end(), '\n' ) );
		if ( first >= count || replacement > count - first ) {
			return invalidateAmbiguous();
		}
		const Uint64 revision = document.getModificationId();
		if ( mRevision == revision ) {
			// Already rebuilt during this edit or a duplicate callback from a shared editor.
			// A pre-mutation query of a structural edit cannot be trusted at this revision.
			if ( mLengths.size() != count ) {
				return invalidateAmbiguous();
			}
		} else {
			if ( revision - mRevision != 1 || last >= mLengths.size() || last < first ||
				 mLengths.size() - ( last - first + 1 ) + replacement != count ) {
				return invalidateAmbiguous();
			}
			if ( replacement != last - first + 1 ) {
				mLengths.erase( mLengths.begin() + first, mLengths.begin() + last + 1 );
				mLengths.insert( mLengths.begin() + first, replacement, 0 );
				for ( size_t line = first; line < first + replacement; ++line )
					mLengths[line] = static_cast<Int64>( document.getLineLength( line ) );
				mLineReads += replacement;
				rebuildBlocks();
				mRevision = revision;
				return !mAmbiguousEdits;
			}
		}
		for ( size_t line = first; line < first + replacement; ++line ) {
			const Int64 length = static_cast<Int64>( document.getLineLength( line ) );
			const Int64 delta = length - mLengths[line];
			mLengths[line] = length;
			for ( size_t block = line / BlockSize + 1; block < mBlocks.size();
				  block += block & ( -block ) )
				mBlocks[block] += delta;
			mTotal += delta;
		}
		mLineReads += replacement;
		mRevision = revision;
		return !mAmbiguousEdits;
	}

	/** Queries can rebuild lengths during a nested notification, but cannot make its event
	 * order trustworthy. Only the manager's frame boundary clears that ambiguity. */
	void clearEditAmbiguity() { mAmbiguousEdits = false; }

	Int32 length() const { return clampOffset( std::max<Int64>( 0, mTotal - 1 ) ); }

	Int32 offset( const Doc::TextPosition& position ) const {
		if ( !position.isValid() || mLengths.empty() )
			return 0;
		const size_t line = std::min<size_t>( position.line(), mLengths.size() - 1 );
		Int64 result = 0;
		const size_t block = line / BlockSize;
		for ( size_t i = block; i; i -= i & ( -i ) )
			result += mBlocks[i];
		for ( size_t i = block * BlockSize; i < line; ++i )
			result += mLengths[i];
		return clampOffset( result + std::min<Int64>( position.column(), mLengths[line] ) );
	}

	Doc::TextPosition position( Int32 offset ) const {
		if ( mLengths.empty() )
			return { 0, 0 };
		Int64 remaining = std::min<Int64>( std::max<Int32>( 0, offset ), mTotal );
		size_t block = 0;
		const size_t blocks = mBlocks.size() - 1;
		// Find the last full-block prefix <= the offset, retaining the final block for EOF.
		for ( size_t step = std::bit_floor( blocks ); step; step >>= 1 ) {
			const size_t next = block + step;
			if ( next < blocks && remaining >= mBlocks[next] ) {
				remaining -= mBlocks[next];
				block = next;
			}
		}
		size_t line = block * BlockSize;
		while ( line + 1 < mLengths.size() && remaining >= mLengths[line] )
			remaining -= mLengths[line++];
		return { static_cast<Int64>( line ), std::min( remaining, mLengths[line] ) };
	}

	/** Counters are updated once per rebuild/edit, not for each query. Used by regression tests
	 * and benchmarks to distinguish indexing work from unrelated editor work. */
	size_t lineReads() const { return mLineReads; }

	size_t rebuilds() const { return mRebuilds; }

	static Int32 clampOffset( Int64 offset ) {
		return static_cast<Int32>(
			std::clamp<Int64>( offset, 0, std::numeric_limits<Int32>::max() ) );
	}

  private:
	System::UUID mDocument{ 0, 0 };
	Uint64 mRevision{};
	std::vector<Int64> mLengths;
	std::vector<Int64> mBlocks;
	Int64 mTotal{};
	size_t mLineReads{};
	size_t mRebuilds{};
	bool mValid{ false };
	bool mAmbiguousEdits{ false };

	bool invalidateAmbiguous() {
		mValid = false;
		mAmbiguousEdits = true;
		return false;
	}

	void rebuildBlocks() {
		mBlocks.assign( 1 + ( mLengths.size() + BlockSize - 1 ) / BlockSize, 0 );
		mTotal = 0;
		for ( size_t line = 0; line < mLengths.size(); ++line ) {
			mBlocks[line / BlockSize + 1] += mLengths[line];
			mTotal += mLengths[line];
		}
		for ( size_t block = 1; block < mBlocks.size(); ++block ) {
			const size_t parent = block + ( block & ( -block ) );
			if ( parent < mBlocks.size() )
				mBlocks[parent] += mBlocks[block];
		}
	}
};

/** One index per queried document in this manager, shared by its editors. Bindings are pointer
 * identities only (never dereferenced). A document switch/deletion drops the last binding; a
 * native disconnect destroys the whole store. No dangling TextDocument keys are possible. */
class AccessibilityTextIndexes {
  public:
	AccessibilityTextIndex& forWidget( const UIWidget* widget, const Doc::TextDocument& document ) {
		for ( auto& entry : mEntries ) {
			if ( entry.document == document.getUUID() &&
				 std::find( entry.users.begin(), entry.users.end(), widget ) !=
					 entry.users.end() ) {
				entry.index.ensure( document );
				return entry.index;
			}
		}
		removeBinding( widget, document.getUUID() );
		for ( auto& entry : mEntries ) {
			if ( entry.document != document.getUUID() )
				continue;
			if ( std::find( entry.users.begin(), entry.users.end(), widget ) == entry.users.end() )
				entry.users.push_back( widget );
			entry.index.ensure( document );
			return entry.index;
		}
		mEntries.push_back( { document.getUUID(), {}, { widget } } );
		mEntries.back().index.ensure( document );
		return mEntries.back().index;
	}

	bool changed( const UIWidget* widget, const Doc::TextDocument& document,
				  const Doc::DocumentContentChange* change ) {
		// A whole-text notification can mean this editor switched documents. Do not retain its
		// old document index until a subsequent text query, or create an index for the new one.
		removeBinding( widget, document.getUUID() );
		for ( auto& entry : mEntries ) {
			if ( entry.document == document.getUUID() )
				return entry.index.changed( document, change );
		}
		return true;
	}

	void detach( const UIWidget* widget ) { removeBinding( widget, std::nullopt ); }

	void clearEditAmbiguity() {
		for ( auto& entry : mEntries )
			entry.index.clearEditAmbiguity();
	}

	size_t size() const { return mEntries.size(); }

  private:
	struct Entry {
		System::UUID document;
		AccessibilityTextIndex index;
		SmallVector<const UIWidget*, 1> users;
	};
	std::vector<Entry> mEntries;

	void removeBinding( const UIWidget* widget, std::optional<System::UUID> keep ) {
		for ( auto it = mEntries.begin(); it != mEntries.end(); ) {
			if ( keep && it->document == *keep ) {
				++it;
				continue;
			}
			auto& users = it->users;
			users.erase( std::remove( users.begin(), users.end(), widget ), users.end() );
			if ( users.empty() )
				it = mEntries.erase( it );
			else
				++it;
		}
	}
};

}} // namespace EE::UI

#endif
