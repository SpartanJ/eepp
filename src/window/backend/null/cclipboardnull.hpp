#ifndef EE_WINDOWCCLIPBOARDNULL_HPP
#define EE_WINDOWCCLIPBOARDNULL_HPP

#include "../../base.hpp"
#include "../../cclipboard.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API cClipboardNull : public cClipboard {
	public:
		virtual ~cClipboardNull();

		std::string GetText();

		std::wstring GetTextWStr();
		
		void SetText( const std::string& Text );
		
		void SetText( const std::wstring& Text );
	protected:
		friend class cWindowNull;

		cClipboardNull( cWindow * window );

		void Init();
};

}}}}

#endif
