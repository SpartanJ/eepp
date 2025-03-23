#pragma once
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/uuid.hpp>
#include <eepp/ui/models/model.hpp>
#include <string>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace EE::UI {
class UISceneNode;
}

namespace ecode {

struct ChatHistory {
	UUID uuid;
	std::string summary;
	FileInfo file;

	ChatHistory( UUID&& uuid, std::string&& summary, FileInfo&& file ) :
		uuid( std::move( uuid ) ), summary( std::move( summary ) ), file( std::move( file ) ) {}

	static std::vector<ChatHistory> getHistory( const std::string& historyFolder );
};

class ChatHistoryModel : public Model {
  public:
	enum Columns { Id, Summary, DateTime, Path, Remove };

	ChatHistoryModel( std::vector<ChatHistory>&&, UISceneNode* );

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const;

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const;

	virtual std::string columnName( const size_t& /*column*/ ) const;

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const;

	void setFilter( const std::string& filter );

	void remove( const ModelIndex& index );

  protected:
	std::vector<const ChatHistory*> mCurHistory;
	std::vector<ChatHistory> mHistory;
	UISceneNode* mUISceneNode;
	std::string mCurFilter;
};

} // namespace ecode
