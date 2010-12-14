#ifndef EECLOG_H
#define EECLOG_H

#include "base.hpp"
#include "tsingleton.hpp"

namespace EE { namespace System {

class EE_API cLog : public tSingleton<cLog> {
	friend class tSingleton<cLog>;
	public:
		void Save(const std::string& filepath = "./");

		void Write(const std::string& Text, const bool& newLine = true);

		void Writef( const char* format, ... );

		std::string Buffer() const { return mData; }

		~cLog();
	protected:
		cLog();
	private:
		std::string mData, mFilePath;
		bool mSave;
};

}}
#endif
