#ifndef __RX_H
#define __RX_H

#include <sys/types.h>
#ifndef NO_POSIX
#include <regex.h>
#else
typedef struct regmatch_t {
    int rm_so;
    int rm_eo;
} regmatch_t;
typedef const char regex_t;
#endif
#ifndef NO_LUA
#include "lua-str.h"
#endif
#include <string>

namespace std {
    string to_string(const string& s);
}

namespace textutil {
    
typedef const std::string& S;
template <class T> inline T from_string(S s) { return s; }
template<> inline int from_string(S s) { return std::stoi(s); }
template<> inline long from_string(S s) { return std::stoi(s); }
template<> inline unsigned long from_string(S s) { return std::stoul(s); }
template<> inline double from_string(S s) { return std::stod(s); }

class Rx {
protected:
    regex_t *rx;

public:

    enum {
        icase = 1, lua = 2, newline = 4
    }; 
    
    struct match_state {
        Rx* pr; 
        regmatch_t *r_matches;
        size_t ref_count;
        bool own_rx;
        
        match_state(Rx* pr, bool own_rx);
        ~match_state();
        
        bool range(int idx, int &i1, int &i2);
        bool matches(const char *s, size_t len);
    };
    
    struct match {
        match_state *state;
        const char *s;
        size_t len;
        
        match(Rx& r, const char *s, bool own_rx=false);
        match(Rx& r, const std::string& s, bool own_rx=false);
        match(const Rx::match& other);
        match& operator= (const match& other);
         ~match();
        
        bool matches() { return state->matches(s,len); }
        bool subst(std::string& res);
        void next();        
        std::string group(int idx = -1) const;
        bool range(int idx, int &i1,int &i2) const { return state->range(idx,i1,i2); }
        std::string operator[] (int idx) const {  return group(idx);  }
        
        template <class C>
        void append_to(C& c) {
            typedef typename C::value_type value_type;
            while(matches()) { 
                c.push_back(from_string<value_type>(group()));
                next();
            }            
        }
        
        template <class M>
        void fill_map(M& m) {
            while(matches()) {
                m[group(1)] = from_string<typename M::mapped_type>(group(2));
                next();
             }
        }

        struct iterator {
            match *pm;

            iterator(match *pm) : pm(pm) {
                if (pm != NULL) {
                    if (! pm->matches()) 
                        pm = NULL; 
                }
            }
            
            bool operator != (const iterator& other) {
                return pm != other.pm;
            }
            
            bool operator == (const iterator& other) {
                return pm == other.pm;
            }            

            const match& operator* () const { return *pm; }

            iterator& operator ++() {
                pm->next();
                if (! pm->matches()) {
                    pm = NULL;
                }
                return *this;
            }
        };


        iterator begin() { return iterator(this);}
        iterator end() { return iterator(NULL); }
    };

    
    Rx() : rx(nullptr) { }
    Rx (Rx&& other);
    Rx (regex_t *R);
    Rx& operator= (Rx&& other);
    
    regex_t *regexp() { return rx; }

    // the above ctor may fail to compile the regex.
    // The suggested idiom is 'if (! R) do_something_with(R.error());'
    virtual bool operator! () { return true; }

    virtual ~Rx() {}
    virtual std::string error() { return ""; }
    virtual int n_matches() { return 0; }
    virtual bool matches (const char *ps, regmatch_t *r_matches = NULL, size_t len = 0) { return false; }
    
    virtual bool matches (const std::string& s, regmatch_t *r_matches = NULL) {
        return matches(s.c_str(),r_matches, s.size());
    }
    
    // like Lua's std::string.find; also returns the range matched
    bool find (const char *str, int& i1, int& i2, int idx = 0);
    bool find (const std::string& s, int& i1, int& i2, int idx = 0) {
        return find(s.c_str(),i1,i2,idx);
    }
    
    bool range(int idx, int& i1, int& i2, regmatch_t *r_matches);
    
    std::string gsub(const char *text, const char *repl);
    std::string gsub(const std::string& text, const std::string& repl) {
        return gsub(text.c_str(),repl.c_str());
    }
    
    match gmatch(const char *s) &;
    match gmatch(const char *s) &&;
    match gmatch(const std::string& s) &;
    match gmatch(const std::string& s) &&;
    
    template <class M>
    std::string gsub(const char *text, M& map_object) {
        Rx::match ms (*this,text);
        std::string res;
        while (ms.subst(res)) {
            res.append(std::to_string(map_object[ms.group()]));
            ms.next();
        }
        return res;
    }
    
    template <class F>
    std::string gsub_fun(const char *text, F fun_object) {
        Rx::match ms (*this,text);
        std::string res;
        while (ms.subst(res)) {
            res.append(std::to_string(fun_object(ms)));
            ms.next();
        }
        return res;
    }

};

#ifndef NO_POSIX
class Rxp: public Rx {
    int rc;
public:    
    Rxp (std::string pat, int cflags = 0);
    Rxp (Rxp&& other) {
        rx = other.rx;
        other.rx = NULL;
    }

    virtual bool operator! () { return rc != 0; }
    virtual ~Rxp();    
    virtual std::string error();
    virtual int n_matches();
    virtual bool matches (const char *ps, regmatch_t *r_matches = NULL,size_t len=0);    
};

inline Rx operator"" _R (const char *pat, size_t) {  return Rxp(pat); }

#endif

#ifndef NO_LUA
class Rxl: public Rx {
    const char *err;
    std::string pat;    
    int n_match;

public:    
    Rxl (std::string pat);
    Rxl (Rxl&& other) {
        rx = other.rx;
        other.rx = NULL;
    }

    virtual bool operator! () { return err != nullptr; }
    virtual ~Rxl();    
    virtual std::string error();
    virtual int n_matches();
    virtual bool matches (const char *ps, regmatch_t *r_matches = NULL,size_t len=0);    
};

inline Rx operator"" _L (const char *pat, size_t) {  return Rxl(pat); }

#endif



}
#endif
