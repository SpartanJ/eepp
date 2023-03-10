#ifndef ECODE_FEATURESHEALTH_HPP
#define ECODE_FEATURESHEALTH_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace ecode {

class PluginManager;

class FeaturesHealth {
  public:
	enum class OutputFormat : int { Ascii, Terminal, Markdown, AsciiDoc };

	static std::unordered_map<std::string, OutputFormat> getMapFlag() {
		return { { "ascii", OutputFormat::Ascii },
				 { "terminal", OutputFormat::Terminal },
				 { "markdown", OutputFormat::Markdown },
				 { "asciidoc", OutputFormat::AsciiDoc } };
	}

	static OutputFormat getDefaultOutputFormat() { return OutputFormat::Terminal; }

	struct FeatureStatus {
		std::string name;
		std::string path;
		std::string url;
		bool found{ false };
	};
	struct LangHealth {
		std::string lang;
		bool syntaxHighlighting{ true };
		FeatureStatus linter;
		FeatureStatus formatter;
		FeatureStatus lsp;
	};

	static std::vector<LangHealth> getHealth( PluginManager* pluginManager,
											  const std::string& lang = "" );

	static std::string generateHealthStatus( PluginManager* pluginManager, OutputFormat format );

	static void doHealth( PluginManager* pluginManager, const std::string& lang = "",
						  const OutputFormat& format = OutputFormat::Terminal );
};

} // namespace ecode

#endif // ECODE_FEATURESHEALTH_HPP
