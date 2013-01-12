#ifndef EE_GAMINGCGAMEOBJECTOBJECT_HPP
#define EE_GAMINGCGAMEOBJECTOBJECT_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/cgameobject.hpp>
#include <eepp/graphics/csubtexture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class cLayer;

class EE_API cGameObjectObject : public cGameObject {
	public:
		typedef std::map<std::string, std::string> PropertiesMap;

		cGameObjectObject( Uint32 DataId, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		cGameObjectObject( Uint32 DataId, const eeRecti& rect, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		virtual ~cGameObjectObject();

		virtual void Draw();

		virtual eeVector2f Pos() const;

		virtual eeSize Size();

		virtual void Pos( eeVector2f pos );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );

		void AddProperty( std::string Text, std::string Value );

		void EditProperty( std::string Text, std::string Value );

		void RemoveProperty( std::string Text );

		void ClearProperties();

		PropertiesMap& GetProperties();
	protected:
		eeRectf			mRect;
		eeVector2f		mPos;
		Uint32			mDataId;
		std::string		mName;
		std::string		mType;
		PropertiesMap	mProperties;
};

}}

#endif
