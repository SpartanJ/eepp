#include <eepp/graphics/textureatlas.hpp>

namespace EE { namespace Graphics {

TextureAtlas::TextureAtlas( const std::string& name ) :
	ResourceManager<SubTexture> ( true )
{
	setName( name );
}

TextureAtlas::~TextureAtlas() {
}

const std::string& TextureAtlas::getName() const {
	return mName;
}

void TextureAtlas::setName( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

const std::string& TextureAtlas::path() const {
	return mPath;
}

void TextureAtlas::path( const std::string& path ) {
	mPath = path;
}

const Uint32& TextureAtlas::getId() const {
	return mId;
}

SubTexture * TextureAtlas::add( SubTexture * subTexture ) {
	return ResourceManager<SubTexture>::add( subTexture );
}

SubTexture * TextureAtlas::add( const Uint32& TexId, const std::string& Name ) {
	return add( eeNew( SubTexture, ( TexId, Name ) ) );
}

SubTexture * TextureAtlas::add( const Uint32& TexId, const Recti& SrcRect, const std::string& Name ) {
	return add( eeNew( SubTexture, ( TexId, SrcRect, Name ) ) );
}

SubTexture * TextureAtlas::add( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const std::string& Name ) {
	return add( eeNew ( SubTexture, ( TexId, SrcRect, DestSize, Name ) ) );
}

SubTexture * TextureAtlas::add( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const Vector2i& Offset, const std::string& Name ) {
	return add( eeNew ( SubTexture, ( TexId, SrcRect, DestSize, Offset, Name ) ) );
}

Uint32 TextureAtlas::count() {
	return ResourceManager<SubTexture>::count();
}

void TextureAtlas::setTextures( std::vector<Texture*> textures ) {
	mTextures = textures;
}

Texture * TextureAtlas::getTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTextures.size() );
	return mTextures[ texnum ];
}

Uint32 TextureAtlas::getTexturesCount() {
	return mTextures.size();
}

}}
