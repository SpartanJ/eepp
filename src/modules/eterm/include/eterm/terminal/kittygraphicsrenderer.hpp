#ifndef ETERM_KITTYGRAPHICSRENDERER_HPP
#define ETERM_KITTYGRAPHICSRENDERER_HPP

#include <eepp/graphics/texture.hpp>
#include <eterm/terminal/terminalgraphics.hpp>
#include <unordered_map>

using namespace EE::Graphics;

namespace eterm { namespace Terminal {

class KittyGraphicsRenderer {
  public:
	enum class Pass : Uint8 { VeryNegative, Negative, NonNegative };

	bool applyUpdates( std::vector<TerminalGraphicsUpdate>&& updates );

	void setPresentation( std::shared_ptr<const TerminalGraphicsPresentation> presentation );

	void draw( Pass pass, const Vector2f& origin, const Sizef& cellSize, const Sizef& gridSize );

	bool hasPlacements( Pass pass ) const;

	void reset();

	Uint64 lastAppliedSequence() const { return mLastAppliedSequence; }

  private:
	struct GPUImage {
		TexturePtr texture;
		std::unordered_map<Uint32, TexturePtr> frames;
		Sizei size;
	};

	std::unordered_map<KittyImageId, GPUImage> mImages;
	std::shared_ptr<const TerminalGraphicsPresentation> mPresentation;
	Uint64 mLastAppliedSequence{ 0 };
};

}} // namespace eterm::Terminal

#endif
