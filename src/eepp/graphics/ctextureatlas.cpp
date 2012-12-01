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

cSubTexture * cTextureAtlas::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name ) {
	return Add( eeNew ( cSubTexture, ( TexId, SrcRect, DestWidth, DestHeight, Name ) ) );
}

cSubTexture * cTextureAtlas::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name ) {
	return Add( eeNew ( cSubTexture, ( TexId, SrcRect, DestWidth, DestHeight, OffsetX, OffsetY, Name ) ) );
}

Uint32 cTextureAtlas::Count() {
	return tResourceManager<cSubTexture>::Count();
}

}}
