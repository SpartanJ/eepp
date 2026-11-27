#ifndef ETERM_KITTYGRAPHICSRENDERER_HPP
#define ETERM_KITTYGRAPHICSRENDERER_HPP

#include <eterm/config.hpp>

#include <eepp/graphics/texture.hpp>
#include <eterm/terminal/terminalgraphics.hpp>
#include <optional>
#include <unordered_map>

using namespace EE::Graphics;

namespace eterm { namespace Terminal {

class EE_ETERM_API KittyGraphicsRenderer {
  public:
	enum class Pass : Uint8 { VeryNegative, Negative, NonNegative };
	enum class UpdateResult : Uint8 { Applied, ResyncNeeded, RecoveryFailed };

	UpdateResult applyUpdates( const std::vector<TerminalGraphicsUpdate>& updates );

	void setPresentation( std::shared_ptr<const TerminalGraphicsPresentation> presentation );

	void draw( Pass pass, const Vector2f& origin, const Sizef& cellSize, const Sizef& gridSize );

	bool hasPlacements( Pass pass ) const;

	Uint64 lastAppliedSequence() const { return mLastAppliedSequence; }

  private:
	struct GPUImage {
		TexturePtr texture;
		std::unordered_map<Uint32, TexturePtr> frames;
		Sizei size;
		Uint8 channels{ 4 };
	};

	using Images = std::unordered_map<KittyImageId, GPUImage>;

	static bool applyUpdate( Images& images, const TerminalGraphicsUpdate& update );

	Images mImages;
	std::optional<Images> mRecoveryImages;
	std::shared_ptr<const TerminalGraphicsPresentation> mPresentation;
	std::shared_ptr<const TerminalGraphicsPresentation> mRecoveryPresentation;
	Uint64 mLastAppliedSequence{ 0 };
	Uint64 mPresentationMinSequence{ 0 };
	Uint64 mRecoverySequence{ 0 };
	Uint64 mRecoveryEndSequence{ 0 };
};

}} // namespace eterm::Terminal

#endif
