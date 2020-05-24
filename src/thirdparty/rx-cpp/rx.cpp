// wrapping POSIX regexes
// Steve Donovan, (c) 2015
// MIT license

#include <string.h>

#include "rx.h"
using namespace std;

const int MAX_DEFAULT_MATCHES = 12;

#ifndef NO_POSIX

static string percent_subst(string pattern) {
    string res;
    const char *p = pattern.c_str();
    bool inside_bracket = false;
    while (*p) {
        char ch = *p;
        if (ch == '%') {
            string klass;
            ++p; // eat special char
            switch(*p) {
                case 's':   klass = "space"; break;
                case 'd':   klass = "digit"; break;
                case 'x':   klass = "xdigit"; break;
                case 'a':   klass = "alpha"; break;
                case 'w':  klass = "alnum"; break;
                case 'u':  klass = "upper"; break;
                case 'l':   klass = "lower"; break;
                case 'c':  klass = "cntrl"; break;
                case 'p':  klass = "punct"; break;
                case '%':
                    ch = '%';
                    break;
                default:
                    ch = '\\';
                    // special case: not a bracket!
                    if (*p == '[' || *p == ']') {
                        res += '\\';
                        ch = *p;
                    } else {
                        --p; // wasn't special after all...
                    }
            }
            if (klass.size() > 0) {
                klass = "[:" + klass + ":]";
                if (! inside_bracket)
                    klass = '[' + klass + ']';
                res += klass;
            } else {
                res += ch;
            }
        } else {
            if (ch == '[')  {
                inside_bracket = true;
            } else
            if (ch == ']') {
                inside_bracket = false;
            }
            res += ch;
        }
        p++;
    }
    return res;
}
#endif

namespace textutil {
    
Rx::Rx (regex_t *R) : rx(R) {
}

Rx::Rx (Rx&& other) {
    rx = other.rx;
    other.rx = NULL;
}

// move assignment as well...
Rx& Rx::operator= (Rx&& other) {
    rx = other.rx;
    other.rx = NULL;
    return *this;
}

// -1 is a special match index: it means, try to pick the first submatch, otherwise
// fall back to the full match. This is the default behaviour for substitutions.
bool Rx::range(int idx, int& i1, int& i2, regmatch_t *r_matches) {
    if (idx == -1) {
        idx = n_matches() > 1 ? 1 : 0;
    }
    if (idx >= 0 && idx < n_matches()) {
        i1 = r_matches[idx].rm_so;
        i2 = r_matches[idx].rm_eo;
        return true;
    }
    return false;
}

// like Lua's string.find; also returns the range matched
bool Rx::find (const char *str, int& i1, int& i2, int idx) {
    regmatch_t match_buff[MAX_DEFAULT_MATCHES];    
    if (matches(str,match_buff)){
        range(idx,i1,i2,match_buff);
        return true;
    } else {
        i1 = -1;
        i2 = -1;
        return false;
    }
}

Rx::match Rx::gmatch(const char *s) &{
    return Rx::match(*this,s,false);
}

Rx::match Rx::gmatch(const std::string& s) & {
    return Rx::match(*this,s.c_str(),false);
}

// some voodoo needed here; this is how we tell whether we were
// created from a _temporary_;  in which case, zero out the regex_t since
// otherwise it will die with the temporary.  The match will make special
// arrangements in this case!
Rx::match Rx::gmatch(const char *s) &&{
    Rx::match M(*this,s,true);
    rx = NULL;
    return M;
}

Rx::match Rx::gmatch(const std::string& s) && {
    Rx::match M(*this,s.c_str(),true);
    rx = NULL;
    return M;    
}

string Rx::gsub(const char *text, const char *repl) {
    Rx::match ms (*this,text);
    string res;
    while (ms.subst(res)) {
        for (const char *P = repl; *P; ++P) {
            if (*P == '%') {
                ++P;
                int ngroup = (int)*P - (int)'0';
                res += ms.group(ngroup);
            } else {
                res += *P;
            }
        }
        ms.next();
    }
    return res; 
}

// a match state looks after the regexp object (a thin wrapper around a regex_t pointer)
// and keeps a buffer for storing the resulting matches.
// If constructed from a temporary Rx, we create our own Rx using its regex_t pointer.
Rx::match_state::match_state(Rx* pr, bool own_rx)  :  ref_count(1), own_rx(own_rx) {
    r_matches = new regmatch_t[10];  // pr->n_matches()
    if (own_rx) {
        this->pr = new Rx(pr->regexp());
    } else {
        this->pr = pr;
    }
}

bool Rx::match_state::range(int idx, int &i1, int &i2) {
    return pr->range(idx,i1,i2,r_matches);
}

bool Rx::match_state::matches(const char *s, size_t len) {
    return pr->matches(s,r_matches,len);
}

Rx::match_state::~match_state() {
    delete[] r_matches;
    if (own_rx)
        delete pr;
}

// A match object keeps a char pointer and a ref-counted match state object
Rx::match::match(Rx& r, const char *s, bool own_rx) : s(s) {
    len = strlen(s);
    state = new Rx::match_state(&r,own_rx);    
}

Rx::match::match(Rx& r, const string& s, bool own_rx) {
    state = new Rx::match_state(&r,own_rx);
    this->s = s.c_str();
    len = s.size();
}

// the match state only dies when there's no state left holding it
Rx::match::~match() {
    --state->ref_count;
    if (state->ref_count == 0) {
        delete state;
    }
}

// so each copied match shares state by incrementing the ref count
Rx::match::match(const Rx::match& other)   : state(other.state), s(other.s), len(other.len) {
    ++state->ref_count;
}

Rx::match& Rx::match::operator= (const match& other) {
    ++state->ref_count;
    return *this;
}

// the match operations are expressed in terms of the basic match_state operations
// matches() & range()

// this moves the char pointer just past the end of the current match
void Rx::match::next() {
    int m1,m2;
    state->range(0,m1,m2);
    s += m2;
    len -= m2;
}

string Rx::match::group(int idx) const {
    int m1,m2;
    if (state->range(idx,m1,m2)) {
        return string(s+m1,m2-m1);
    }
    return "";
}

bool Rx::match::subst(string& res) {
    if (! matches()) { // copy remaining tail
        res.append(s);
        return false;
    }
    int m1,m2;
    state->range(0,m1,m2);
    if (m1 == 0)
        return true;
    res.append(s,m1);
    return true;
}

#ifndef NO_POSIX

// Rxp just wraps a regex_t struct
Rxp::Rxp (string pat, int cflags)  : Rx() {
    rx = new regex_t;
    int flags = REG_EXTENDED;
    if (cflags & icase)
        flags |= REG_ICASE;
    if (cflags & newline)
        flags |= REG_NEWLINE;
    if (cflags & lua)
        pat = percent_subst(pat);
    rc = regcomp(rx, pat.c_str(), flags);
}

Rxp::~Rxp() {
    if (rx) {
        regfree(rx);
        delete rx;
    }
}

// if the regexp compilation fails, then use this;
// if (! R) { do_something_with(R.error()); }
string Rxp::error() {
    char buff[512];
    regerror(rc,rx,buff,sizeof(buff));
    return buff;
}

// basic match operation!
bool Rxp::matches (const char *ps, regmatch_t *r_matches, size_t len) {
    regmatch_t match_buff[MAX_DEFAULT_MATCHES];
    if (r_matches == NULL)
        r_matches = match_buff;
    int res = regexec(rx,ps, n_matches(), r_matches,0);
    return res != 0 ? false : true;
}

int Rxp::n_matches() {
    return rx->re_nsub+1;
}

#endif

#ifndef NO_LUA

static bool s_handler_initialized;

static void fail_handler (const char *msg) {
    throw string(msg);
}

static const char *copy_str(const string& s) {
    char *out = new char[s.size()+1];
    strcpy(out,s.c_str());
    return out;
}

// Rxp just wraps a pattern string
Rxl::Rxl (string pat)  : Rx(), err(nullptr), pat(pat) {
    //rx = (regex_t*)copy_str(pat); // PASOP
    rx = (regex_t*)pat.c_str();
    if (! s_handler_initialized) {
        s_handler_initialized = true;
        str_fail_func(fail_handler);
    }
    n_match = 10;
}

Rxl::~Rxl() {
    //delete[] rx;
    if (err)
        delete[] err;
}

// if the regexp compilation fails, then use this;
// if (! R) { do_something_with(R.error()); }
string Rxl::error() {
    return err;
}

// basic match operation!
bool Rxl::matches (const char *ps, regmatch_t *r_matches, size_t len) {
    regmatch_t match_buff[MAX_DEFAULT_MATCHES];
    if (r_matches == NULL)
        r_matches = match_buff;
    if (len == 0)
        len = strlen(ps);
    // if there's an error in the pattern, it will throw and set error state
    // LuaMatch and regmatch_t are just the same under the hood
    try {
        n_match = str_match(ps,0,len,pat.c_str(), (LuaMatch*) r_matches);
    } catch (const string& pattern_error) {
        err = copy_str(pattern_error);
        n_match = 0;
    }
    return n_match == 0 ? false : true;
}

bool Rxl::matches (const char *ps, int offset, regmatch_t *r_matches, size_t len) {
    regmatch_t match_buff[MAX_DEFAULT_MATCHES];
    if (r_matches == NULL)
        r_matches = match_buff;
    if (len == 0)
        len = strlen(ps);
    // if there's an error in the pattern, it will throw and set error state
    // LuaMatch and regmatch_t are just the same under the hood
    try {
        n_match = str_match(ps, offset,len,pat.c_str(), (LuaMatch*) r_matches);
    } catch (const string& pattern_error) {
        err = copy_str(pattern_error);
        n_match = 0;
    }
    return n_match == 0 ? false : true;
}

bool Rxl::find(const char* str, int offset, int& i1, int& i2, int len, int idx) {
    regmatch_t match_buff[MAX_DEFAULT_MATCHES];
    if (matches(str,offset,match_buff,len)){
        range(idx,i1,i2,match_buff);
        return true;
    } else {
        i1 = -1;
        i2 = -1;
        return false;
    }
}

int Rxl::n_matches() {
    return n_match;
}

#endif


}

// overload to_string so that we can use it in gsub with any convertable type
namespace std {
    string to_string(const string& s) {  return s; }
}

