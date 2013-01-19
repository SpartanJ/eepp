#include <eepp/graphics/ctextureatlas.hpp>

namespace EE { namespace Graphics {

cTextureAtlas::cTextureAtlas( const std::string& name ) :
	tResourceManager<cSubTexture> ( true )
{
	Name( name );
}

cTextureAtlas::~cTextureAtlas() {
}

const std::string& cTextureAtlas::Name() const {
	return mName;
}

void cTextureAtlas::Name( const std::string& name ) {
	mName = name;
	mId = String::Hash( mName );
}

const std::string& cTextureAtlas::Path() const {
	return mPath;
}

void cTextureAtlas::Path( const std::string& path ) {
	mPath = path;
}

const Uint32& cTextureAtlas::Id() const {
	return mId;
}

cSubTexture * cTextureAtlas::Add( cSubTexture * subTexture ) {
	return tResourceManager<cSubTexture>::Add( subTexture );
}

cSubTexture * cTextureAtlas::Add( const Uint32& TexId, const std::string& Name ) {
	return Add( eeNew( cSubTexture, ( TexId, Name ) ) );
}

cSubTexture * cTextureAtlas::Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name ) {
	return Add( eeNew( cSubTexture, ( TexId, SrcRect, Name ) ) );
}

cSubTexture * cTextureAtlas::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeSizef& DestSize, const std::string& Name ) {
	return Add( eeNew ( cSubTexture, ( TexId, SrcRect, DestSize, Name ) ) );
}

cSubTexture * cTextureAtlas::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeSizef& DestSize, const eeVector2i& Offset, const std::string& Name ) {
	return Add( eeNew ( cSubTexture, ( TexId, SrcRect, DestSize, Offset, Name ) ) );
}

Uint32 cTextureAtlas::Count() {
	return tResourceManager<cSubTexture>::Count();
}

void cTextureAtlas::SetTextures( std::vector<cTexture*> textures ) {
	mTextures = textures;
}

cTexture * cTextureAtlas::GetTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTextures.size() );
	return mTextures[ texnum ];
}

Uint32 cTextureAtlas::GetTexturesCount() {
	return mTextures.size();
}

}}
