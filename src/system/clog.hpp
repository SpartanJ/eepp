#ifndef EECLOG_H
#define EECLOG_H

#include "base.hpp"
#include "singleton.hpp"

namespace EE { namespace System {

class EE_API cLog : public cSingleton<cLog> {
	friend class cSingleton<cLog>;
	public:
		void Save(const std::string& filepath = "./");
		void Write(const std::string& Text, const bool& newLine = true);
		void Writef( const char* format, ... );
		std::string Buffer() const { return mData; }
	protected:
		cLog();
		~cLog();
	private:
		std::string mData, mFilePath;
		bool mSave;
};

}}
#endif
