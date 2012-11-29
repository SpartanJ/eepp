#include <eepp/graphics/cshapegroupmanager.hpp>
#include <eepp/graphics/cglobalshapegroup.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cShapeGroupManager)

cShapeGroupManager::cShapeGroupManager() :
	tResourceManager<cShapeGroup>( false ),
	mWarnings( false )
{
	Add( cGlobalShapeGroup::instance() );
}

cShapeGroupManager::~cShapeGroupManager() {
}

cShape * cShapeGroupManager::GetShapeByName( const std::string& Name ) {
	cShape * tShape = GetShapeById( MakeHash( Name ) );

	if ( mWarnings ) {
		eePRINTC( NULL == tShape, "cShapeGroupManager::GetShapeByName shape '%s' not found\n", Name.c_str() );
	}

	return tShape;
}

cShape * cShapeGroupManager::GetShapeById( const Uint32& Id ) {
	std::list<cShapeGroup*>::iterator it;

	cShapeGroup * tSG = NULL;
	cShape * tShape = NULL;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		tSG = (*it);

		tShape = tSG->GetById( Id );

		if ( NULL != tShape )
			return tShape;
	}

	return NULL;
}

void cShapeGroupManager::PrintResources() {
	std::list<cShapeGroup*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->PrintNames();
}

std::vector<cShape*> cShapeGroupManager::GetShapesByPatternId( const Uint32& ShapeId, const std::string& extension, cShapeGroup * SearchInShapeGroup ) {
	cShape * tShape 	= NULL;
	std::string tName;

	if ( NULL == SearchInShapeGroup )
		tShape = GetShapeById( ShapeId );
	else
		tShape = SearchInShapeGroup->GetById( ShapeId );

	if ( NULL != tShape ) {
		if ( extension.size() )
			tName = String::RemoveNumbersAtEnd( FileSystem::FileRemoveExtension( tShape->Name() ) ) + extension;
		else
			tName = tShape->Name();

		return GetShapesByPattern( String::RemoveNumbersAtEnd( tShape->Name() ), "", SearchInShapeGroup );
	}

	return std::vector<cShape*>();
}

void cShapeGroupManager::PrintWarnings( const bool& warn ) {
	mWarnings = warn;
}

const bool& cShapeGroupManager::PrintWarnings() const {
	return mWarnings;
}

std::vector<cShape*> cShapeGroupManager::GetShapesByPattern( const std::string& name, const std::string& extension, cShapeGroup * SearchInShapeGroup ) {
	std::vector<cShape*> 	Shapes;
	std::string 			search;
	bool 					found 	= true;
	cShape *				tShape 	= NULL;
	std::string				realext = "";
	eeInt 					c 		= 0;
	eeInt					t		= 0;
	eeInt i;

	if ( extension.size() )
		realext = "." + extension;

	// Test if name starts with 0 - 1
	for ( i = 0; i < 2; i++ ) {
		search = String::StrFormated( "%s%d%s", name.c_str(), i, realext.c_str() );

		if ( NULL == SearchInShapeGroup )
			tShape = GetShapeByName( search );
		else
			tShape = SearchInShapeGroup->GetByName( search );

		if ( NULL != tShape ) {
			t = 1;

			break;
		}
	}

	// in case that name doesn't start with 0 - 1, we test with 00 - 01
	if ( 0 == t ) {
		for ( i = 0; i < 2; i++ ) {
			search = String::StrFormated( "%s%02d%s", name.c_str(), i, realext.c_str() );

			if ( NULL == SearchInShapeGroup )
				tShape = GetShapeByName( search );
			else
				tShape = SearchInShapeGroup->GetByName( search );

			if ( NULL != tShape ) {
				t = 2;

				break;
			}
		}

		// in case that name doesn't start with 00 - 01, we test with 000 - 001
		if ( 0 == t ) {
			for ( i = 0; i < 2; i++ ) {
				search = String::StrFormated( "%s%03d%s", name.c_str(), i, realext.c_str() );

				if ( NULL == SearchInShapeGroup )
					tShape = GetShapeByName( search );
				else
					tShape = SearchInShapeGroup->GetByName( search );

				if ( NULL != tShape ) {
					t = 3;

					break;
				}
			}

			if ( 0 == t ) {
				for ( i = 0; i < 2; i++ ) {
					search = String::StrFormated( "%s%04d%s", name.c_str(), i, realext.c_str() );

					if ( NULL == SearchInShapeGroup )
						tShape = GetShapeByName( search );
					else
						tShape = SearchInShapeGroup->GetByName( search );

					if ( NULL != tShape ) {
						t = 4;

						break;
					}
				}

				if ( 0 == t ) {
					for ( i = 0; i < 2; i++ ) {
						search = String::StrFormated( "%s%05d%s", name.c_str(), i, realext.c_str() );

						if ( NULL == SearchInShapeGroup )
							tShape = GetShapeByName( search );
						else
							tShape = SearchInShapeGroup->GetByName( search );

						if ( NULL != tShape ) {
							t = 5;

							break;
						}
					}

					if ( 0 == t ) {
						for ( i = 0; i < 2; i++ ) {
							search = String::StrFormated( "%s%06d%s", name.c_str(), i, realext.c_str() );

							if ( NULL == SearchInShapeGroup )
								tShape = GetShapeByName( search );
							else
								tShape = SearchInShapeGroup->GetByName( search );

							if ( NULL != tShape ) {
								t = 6;

								break;
							}
						}
					}
				}
			}
		}
	}

	if ( 0 != t ) {
		do {
			switch ( t ) {
				case 1: search = String::StrFormated( "%s%d%s", name.c_str(), c, realext.c_str() ); break;
				case 2: search = String::StrFormated( "%s%02d%s", name.c_str(), c, realext.c_str() ); break;
				case 3: search = String::StrFormated( "%s%03d%s", name.c_str(), c, realext.c_str() ); break;
				case 4: search = String::StrFormated( "%s%04d%s", name.c_str(), c, realext.c_str() ); break;
				case 5: search = String::StrFormated( "%s%05d%s", name.c_str(), c, realext.c_str() ); break;
				case 6: search = String::StrFormated( "%s%06d%s", name.c_str(), c, realext.c_str() ); break;
				default: found = false;
			}

			if ( found ) {
				if ( NULL == SearchInShapeGroup )
					tShape = GetShapeByName( search );
				else
					tShape = SearchInShapeGroup->GetByName( search );

				if ( NULL != tShape ) {
					Shapes.push_back( tShape );

					found = true;
				} else {
					if ( 0 == c ) // if didn't found "00", will search at least for "01"
						found = true;
					else
						found = false;
				}
			}

			c++;
		} while ( found );
	}

	return Shapes;
}

}}
