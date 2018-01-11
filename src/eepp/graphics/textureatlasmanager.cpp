#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(TextureAtlasManager)

TextureAtlasManager::TextureAtlasManager() :
	ResourceManager<TextureAtlas>( false ),
	mWarnings( false )
{
	add( GlobalTextureAtlas::instance() );
}

TextureAtlasManager::~TextureAtlasManager() {
}

TextureAtlas * TextureAtlasManager::loadFromFile( const std::string& TextureAtlasPath ) {
	TextureAtlasLoader loader( TextureAtlasPath );

	return loader.getTextureAtlas();
}

TextureAtlas * TextureAtlasManager::loadFromStream( IOStream& IOS ) {
	TextureAtlasLoader loader( IOS );

	return loader.getTextureAtlas();
}

TextureAtlas * TextureAtlasManager::loadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName ) {
	TextureAtlasLoader loader( Data, DataSize, TextureAtlasName );

	return loader.getTextureAtlas();
}

TextureAtlas * TextureAtlasManager::loadFromPack( Pack * Pack, const std::string& FilePackPath ) {
	TextureAtlasLoader loader( Pack, FilePackPath );

	return loader.getTextureAtlas();
}

SubTexture * TextureAtlasManager::getSubTextureByName( const std::string& Name ) {
	SubTexture * tSubTexture = getSubTextureById( String::hash( Name ) );

	if ( mWarnings ) {
		eePRINTC( NULL == tSubTexture, "TextureAtlasManager::GetSubTextureByName SubTexture '%s' not found\n", Name.c_str() );
	}

	return tSubTexture;
}

SubTexture * TextureAtlasManager::getSubTextureById( const Uint32& Id ) {
	std::list<TextureAtlas*>::iterator it;

	TextureAtlas * tSG = NULL;
	SubTexture * tSubTexture = NULL;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		tSG = (*it);

		tSubTexture = tSG->getById( Id );

		if ( NULL != tSubTexture )
			return tSubTexture;
	}

	return NULL;
}

void TextureAtlasManager::printResources() {
	std::list<TextureAtlas*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->printNames();
}

std::vector<SubTexture*> TextureAtlasManager::getSubTexturesByPatternId( const Uint32& SubTextureId, const std::string& extension, TextureAtlas * SearchInTextureAtlas ) {
	SubTexture * tSubTexture 	= NULL;
	std::string tName;

	if ( NULL == SearchInTextureAtlas )
		tSubTexture = getSubTextureById( SubTextureId );
	else
		tSubTexture = SearchInTextureAtlas->getById( SubTextureId );

	if ( NULL != tSubTexture ) {
		if ( extension.size() )
			tName = String::removeNumbersAtEnd( FileSystem::fileRemoveExtension( tSubTexture->getName() ) ) + extension;
		else
			tName = tSubTexture->getName();

		return getSubTexturesByPattern( String::removeNumbersAtEnd( tSubTexture->getName() ), "", SearchInTextureAtlas );
	}

	return std::vector<SubTexture*>();
}

void TextureAtlasManager::setPrintWarnings( const bool& warn ) {
	mWarnings = warn;
}

const bool& TextureAtlasManager::getPrintWarnings() const {
	return mWarnings;
}

std::vector<SubTexture*> TextureAtlasManager::getSubTexturesByPattern( const std::string& name, const std::string& extension, TextureAtlas * SearchInTextureAtlas ) {
	std::vector<SubTexture*> 	SubTextures;
	std::string 			search;
	bool 					found 	= true;
	SubTexture *				tSubTexture 	= NULL;
	std::string				realext = "";
	int 					c 		= 0;
	int					t		= 0;
	int i;

	if ( extension.size() )
		realext = "." + extension;

	// Test if name starts with 0 - 1
	for ( i = 0; i < 2; i++ ) {
		search = String::strFormated( "%s%d%s", name.c_str(), i, realext.c_str() );

		if ( NULL == SearchInTextureAtlas )
			tSubTexture = getSubTextureByName( search );
		else
			tSubTexture = SearchInTextureAtlas->getByName( search );

		if ( NULL != tSubTexture ) {
			t = 1;

			break;
		}
	}

	// in case that name doesn't start with 0 - 1, we test with 00 - 01
	if ( 0 == t ) {
		for ( i = 0; i < 2; i++ ) {
			search = String::strFormated( "%s%02d%s", name.c_str(), i, realext.c_str() );

			if ( NULL == SearchInTextureAtlas )
				tSubTexture = getSubTextureByName( search );
			else
				tSubTexture = SearchInTextureAtlas->getByName( search );

			if ( NULL != tSubTexture ) {
				t = 2;

				break;
			}
		}

		// in case that name doesn't start with 00 - 01, we test with 000 - 001
		if ( 0 == t ) {
			for ( i = 0; i < 2; i++ ) {
				search = String::strFormated( "%s%03d%s", name.c_str(), i, realext.c_str() );

				if ( NULL == SearchInTextureAtlas )
					tSubTexture = getSubTextureByName( search );
				else
					tSubTexture = SearchInTextureAtlas->getByName( search );

				if ( NULL != tSubTexture ) {
					t = 3;

					break;
				}
			}

			if ( 0 == t ) {
				for ( i = 0; i < 2; i++ ) {
					search = String::strFormated( "%s%04d%s", name.c_str(), i, realext.c_str() );

					if ( NULL == SearchInTextureAtlas )
						tSubTexture = getSubTextureByName( search );
					else
						tSubTexture = SearchInTextureAtlas->getByName( search );

					if ( NULL != tSubTexture ) {
						t = 4;

						break;
					}
				}

				if ( 0 == t ) {
					for ( i = 0; i < 2; i++ ) {
						search = String::strFormated( "%s%05d%s", name.c_str(), i, realext.c_str() );

						if ( NULL == SearchInTextureAtlas )
							tSubTexture = getSubTextureByName( search );
						else
							tSubTexture = SearchInTextureAtlas->getByName( search );

						if ( NULL != tSubTexture ) {
							t = 5;

							break;
						}
					}

					if ( 0 == t ) {
						for ( i = 0; i < 2; i++ ) {
							search = String::strFormated( "%s%06d%s", name.c_str(), i, realext.c_str() );

							if ( NULL == SearchInTextureAtlas )
								tSubTexture = getSubTextureByName( search );
							else
								tSubTexture = SearchInTextureAtlas->getByName( search );

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
				case 1: search = String::strFormated( "%s%d%s", name.c_str(), c, realext.c_str() ); break;
				case 2: search = String::strFormated( "%s%02d%s", name.c_str(), c, realext.c_str() ); break;
				case 3: search = String::strFormated( "%s%03d%s", name.c_str(), c, realext.c_str() ); break;
				case 4: search = String::strFormated( "%s%04d%s", name.c_str(), c, realext.c_str() ); break;
				case 5: search = String::strFormated( "%s%05d%s", name.c_str(), c, realext.c_str() ); break;
				case 6: search = String::strFormated( "%s%06d%s", name.c_str(), c, realext.c_str() ); break;
				default: found = false;
			}

			if ( found ) {
				if ( NULL == SearchInTextureAtlas )
					tSubTexture = getSubTextureByName( search );
				else
					tSubTexture = SearchInTextureAtlas->getByName( search );

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
