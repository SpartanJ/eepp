#include <eepp/graphics/textureatlas.hpp>

namespace EE { namespace Graphics {

TextureAtlas::TextureAtlas( const std::string& name ) :
	ResourceManager<DrawableResource> ( true )
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

const std::string& TextureAtlas::getPath() const {
	return mPath;
}

void TextureAtlas::setPath( const std::string& path ) {
	mPath = path;
}

const Uint32& TextureAtlas::getId() const {
	return mId;
}

DrawableResource * TextureAtlas::add( DrawableResource * drawableResource ) {
	return ResourceManager<DrawableResource>::add( drawableResource );
}

DrawableResource * TextureAtlas::add( const Uint32& TexId, const std::string& Name ) {
	return add( eeNew( SubTexture, ( TexId, Name ) ) );
}

DrawableResource * TextureAtlas::add( const Uint32& TexId, const Rect& SrcRect, const std::string& Name ) {
	return add( eeNew( SubTexture, ( TexId, SrcRect, Name ) ) );
}

DrawableResource * TextureAtlas::add( const Uint32& TexId, const Rect& SrcRect, const Sizef& DestSize, const std::string& Name ) {
	return add( eeNew ( SubTexture, ( TexId, SrcRect, DestSize, Name ) ) );
}

DrawableResource * TextureAtlas::add( const Uint32& TexId, const Rect& SrcRect, const Sizef& DestSize, const Vector2i& Offset, const std::string& Name ) {
	return add( eeNew ( SubTexture, ( TexId, SrcRect, DestSize, Offset, Name ) ) );
}

Uint32 TextureAtlas::getCount() {
	return ResourceManager<DrawableResource>::getCount();
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
