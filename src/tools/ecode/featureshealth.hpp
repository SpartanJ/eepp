#ifndef ECODE_FEATURESHEALTH_HPP
#define ECODE_FEATURESHEALTH_HPP

#include <string>
#include <vector>

namespace ecode {

class PluginManager;

class FeaturesHealth {
  public:
	struct FeatureStatus {
		std::string name;
		std::string path;
		bool found{ false };
	};
	struct LangHealth {
		std::string lang;
		bool syntaxHighlighting{ true };
		FeatureStatus linter;
		FeatureStatus formatter;
		FeatureStatus lsp;
	};

	static std::vector<LangHealth> getHealth( PluginManager* pluginManager );

	static void printHealth( PluginManager* pluginManager );
};

} // namespace ecode

#endif // ECODE_FEATURESHEALTH_HPP
