#ifndef EPROC_POSIX_PROCESS_ACCOUNT_HPP
#define EPROC_POSIX_PROCESS_ACCOUNT_HPP

#include <array>
#include <cstring>
#include <pwd.h>
#include <string>
#include <unordered_map>
#include <utility>

namespace eproc {

/** Cached POSIX account metadata shared by the macOS and FreeBSD collectors. */
class ProcessAccounts {
  public:
	struct Account {
		std::string name;
		bool canLogin{ false };
	};

	const Account& get( uid_t uid ) {
		auto found = mAccounts.find( uid );
		if ( found != mAccounts.end() )
			return found->second;
		Account account;
		std::array<char, 16384> buffer;
		passwd pwd{};
		passwd* result = nullptr;
		if ( getpwuid_r( uid, &pwd, buffer.data(), buffer.size(), &result ) == 0 && result ) {
			account.name = pwd.pw_name;
			const char* shell = pwd.pw_shell;
			account.canLogin =
				shell && *shell && !strstr( shell, "nologin" ) && !strstr( shell, "/false" );
		}
		return mAccounts.emplace( uid, std::move( account ) ).first->second;
	}

  private:
	std::unordered_map<uid_t, Account> mAccounts;
};

} // namespace eproc

#endif // EPROC_POSIX_PROCESS_ACCOUNT_HPP
