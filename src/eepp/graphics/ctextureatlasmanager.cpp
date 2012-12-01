#include <eepp/graphics/ctextureatlasmanager.hpp>
#include <eepp/graphics/cglobaltextureatlas.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cTextureAtlasManager)

cTextureAtlasManager::cTextureAtlasManager() :
	tResourceManager<cTextureAtlas>( false ),
	mWarnings( false )
{
	Add( cGlobalTextureAtlas::instance() );
}

cTextureAtlasManager::~cTextureAtlasManager() {
}

cSubTexture * cTextureAtlasManager::GetSubTextureByName( const std::string& Name ) {
	cSubTexture * tSubTexture = GetSubTextureById( String::Hash( Name ) );

	if ( mWarnings ) {
		eePRINTC( NULL == tSubTexture, "cTextureAtlasManager::GetSubTextureByName SubTexture '%s' not found\n", Name.c_str() );
	}

	return tSubTexture;
}

cSubTexture * cTextureAtlasManager::GetSubTextureById( const Uint32& Id ) {
	std::list<cTextureAtlas*>::iterator it;

	cTextureAtlas * tSG = NULL;
	cSubTexture * tSubTexture = NULL;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		tSG = (*it);

		tSubTexture = tSG->GetById( Id );

		if ( NULL != tSubTexture )
			return tSubTexture;
	}

	return NULL;
}

void cTextureAtlasManager::PrintResources() {
	std::list<cTextureAtlas*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->PrintNames();
}

std::vector<cSubTexture*> cTextureAtlasManager::GetSubTexturesByPatternId( const Uint32& SubTextureId, const std::string& extension, cTextureAtlas * SearchInTextureAtlas ) {
	cSubTexture * tSubTexture 	= NULL;
	std::string tName;

	if ( NULL == SearchInTextureAtlas )
		tSubTexture = GetSubTextureById( SubTextureId );
	else
		tSubTexture = SearchInTextureAtlas->GetById( SubTextureId );

	if ( NULL != tSubTexture ) {
		if ( extension.size() )
			tName = String::RemoveNumbersAtEnd( FileSystem::FileRemoveExtension( tSubTexture->Name() ) ) + extension;
		else
			tName = tSubTexture->Name();

		return GetSubTexturesByPattern( String::RemoveNumbersAtEnd( tSubTexture->Name() ), "", SearchInTextureAtlas );
	}

	return std::vector<cSubTexture*>();
}

void cTextureAtlasManager::PrintWarnings( const bool& warn ) {
	mWarnings = warn;
}

const bool& cTextureAtlasManager::PrintWarnings() const {
	return mWarnings;
}

std::vector<cSubTexture*> cTextureAtlasManager::GetSubTexturesByPattern( const std::string& name, const std::string& extension, cTextureAtlas * SearchInTextureAtlas ) {
	std::vector<cSubTexture*> 	SubTextures;
	std::string 			search;
	bool 					found 	= true;
	cSubTexture *				tSubTexture 	= NULL;
	std::string				realext = "";
	eeInt 					c 		= 0;
	eeInt					t		= 0;
	eeInt i;

	if ( extension.size() )
		realext = "." + extension;

	// Test if name starts with 0 - 1
	for ( i = 0; i < 2; i++ ) {
		search = String::StrFormated( "%s%d%s", name.c_str(), i, realext.c_str() );

		if ( NULL == SearchInTextureAtlas )
			tSubTexture = GetSubTextureByName( search );
		else
			tSubTexture = SearchInTextureAtlas->GetByName( search );

		if ( NULL != tSubTexture ) {
			t = 1;

			break;
		}
	}

	// in case that name doesn't start with 0 - 1, we test with 00 - 01
	if ( 0 == t ) {
		for ( i = 0; i < 2; i++ ) {
			search = String::StrFormated( "%s%02d%s", name.c_str(), i, realext.c_str() );

			if ( NULL == SearchInTextureAtlas )
				tSubTexture = GetSubTextureByName( search );
			else
				tSubTexture = SearchInTextureAtlas->GetByName( search );

			if ( NULL != tSubTexture ) {
				t = 2;

				break;
			}
		}

		// in case that name doesn't start with 00 - 01, we test with 000 - 001
		if ( 0 == t ) {
			for ( i = 0; i < 2; i++ ) {
				search = String::StrFormated( "%s%03d%s", name.c_str(), i, realext.c_str() );

				if ( NULL == SearchInTextureAtlas )
					tSubTexture = GetSubTextureByName( search );
				else
					tSubTexture = SearchInTextureAtlas->GetByName( search );

				if ( NULL != tSubTexture ) {
					t = 3;

					break;
				}
			}

			if ( 0 == t ) {
				for ( i = 0; i < 2; i++ ) {
					search = String::StrFormated( "%s%04d%s", name.c_str(), i, realext.c_str() );

					if ( NULL == SearchInTextureAtlas )
						tSubTexture = GetSubTextureByName( search );
					else
						tSubTexture = SearchInTextureAtlas->GetByName( search );

					if ( NULL != tSubTexture ) {
						t = 4;

						break;
					}
				}

				if ( 0 == t ) {
					for ( i = 0; i < 2; i++ ) {
						search = String::StrFormated( "%s%05d%s", name.c_str(), i, realext.c_str() );

						if ( NULL == SearchInTextureAtlas )
							tSubTexture = GetSubTextureByName( search );
						else
							tSubTexture = SearchInTextureAtlas->GetByName( search );

						if ( NULL != tSubTexture ) {
							t = 5;

							break;
						}
					}

					if ( 0 == t ) {
						for ( i = 0; i < 2; i++ ) {
							search = String::StrFormated( "%s%06d%s", name.c_str(), i, realext.c_str() );

							if ( NULL == SearchInTextureAtlas )
								tSubTexture = GetSubTextureByName( search );
							else
								tSubTexture = SearchInTextureAtlas->GetByName( search );

							if ( NULL != tSubTexture ) {
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
				if ( NULL == SearchInTextureAtlas )
					tSubTexture = GetSubTextureByName( search );
				else
					tSubTexture = SearchInTextureAtlas->GetByName( search );

				if ( NULL != tSubTexture ) {
					SubTextures.push_back( tSubTexture );

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

	return SubTextures;
}

}}
