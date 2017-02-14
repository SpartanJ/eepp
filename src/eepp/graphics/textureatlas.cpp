#include <eepp/graphics/textureatlas.hpp>

namespace EE { namespace Graphics {

TextureAtlas::TextureAtlas( const std::string& name ) :
	ResourceManager<SubTexture> ( true )
{
	Name( name );
}

TextureAtlas::~TextureAtlas() {
}

const std::string& TextureAtlas::Name() const {
	return mName;
}

void TextureAtlas::Name( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

const std::string& TextureAtlas::Path() const {
	return mPath;
}

void TextureAtlas::Path( const std::string& path ) {
	mPath = path;
}

const Uint32& TextureAtlas::Id() const {
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

void TextureAtlas::SetTextures( std::vector<Texture*> textures ) {
	mTextures = textures;
}

Texture * TextureAtlas::GetTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTextures.size() );
	return mTextures[ texnum ];
}

Uint32 TextureAtlas::GetTexturesCount() {
	return mTextures.size();
}

}}
