#ifndef EE_UI_UICLIP_HPP
#define EE_UI_UICLIP_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace UI {

enum class ClipType : Uint32 { None, ContentBox, PaddingBox, BorderBox };

class UIClip {
  public:
	UIClip();

	UIClip( const ClipType& clipType );

	static ClipType fromString( std::string str );

	static std::string toString( const ClipType& clipType );

	const ClipType& getClipType() const;

	void setClipType( const ClipType& clipType );

  protected:
	ClipType mClipType{ ClipType::None };
};

}} // namespace EE::UI

#endif // EE_UI_UICLIP_HPP
