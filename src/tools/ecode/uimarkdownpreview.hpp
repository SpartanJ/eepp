#ifndef ECODE_UIMARKDOWNPREVIEW_HPP
#define ECODE_UIMARKDOWNPREVIEW_HPP

#include <eepp/scene/eventconnection.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uimarkdownview.hpp>

using namespace EE::UI;

namespace ecode {

/** A preview tab keeps its original live source independently of the document reached by links. */
class UIMarkdownPreview : public UIScrollableMarkdownView {
  public:
	explicit UIMarkdownPreview( std::string sourcePath );

	const std::string& getSourcePath() const;

	void bindSource( UICodeEditor* editor );

  protected:
	virtual void loadHistoryDocument( const std::string& path );

  private:
	std::string mSourcePath;
	std::weak_ptr<TextDocument> mSourceDocument;
	EventConnection mTextChangedConnection;

	void updateFromSource();
};

} // namespace ecode

#endif
