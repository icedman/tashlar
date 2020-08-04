#include "textmate.h"

#include "grammar.h"
#include "parse.h"
#include "reader.h"
#include "theme.h"

#include <iostream>

struct tm_state_t {
    int id;
};

struct tm_grammar_t {
    parse::grammar_ptr _grammar;
};

struct tm_theme_t {
    theme_ptr _theme;
};

void tm_init(struct textmate_t* tm)
{
    tm->grammar = (struct tm_grammar_t*)calloc(1, sizeof(struct tm_grammar_t));
    tm->theme = (struct tm_theme_t*)calloc(1, sizeof(struct tm_theme_t));
    tm->state = (struct tm_state_t*)calloc(1, sizeof(struct tm_state_t));
}

void tm_free(struct textmate_t* tm)
{
    free(tm->grammar);
    free(tm->theme);
    free(tm->state);
}

void tm_load_grammar(struct textmate_t* tm, char* filename)
{
    Json::Value json = parse::loadJson(filename);
    tm->grammar->_grammar = parse::parse_grammar(json);
    // std::cout << "load " << json << std::endl;
}

void tm_load_theme(struct textmate_t* tm, char* filename)
{
    Json::Value json = parse::loadJson(filename);
    tm->theme->_theme = parse_theme(json);
    // std::cout << "load " << json << std::endl;
}

void dump_tokens(std::map<size_t, scope::scope_t>& scopes)
{
    FILE* fp = fopen("./out.txt", "w");
    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    while (it != scopes.end()) {
        size_t n = it->first;
        scope::scope_t scope = it->second;
        // std::cout << n << " size:" << scope.size() << " atoms:"
        //           << to_s(scope).c_str()
        //           << std::endl;
        fprintf(fp, "%s\n", to_s(scope).c_str());
        it++;
    }
    fclose(fp);
}

void tm_parse_line(struct textmate_t* tm, char const* first, char const* last)
{
    parse::stack_ptr parser_state = tm->grammar->_grammar->seed();
    std::map<size_t, scope::scope_t> scopes;
    parser_state = parse::parse(first, last, parser_state, scopes, true);

    // todo save to state

    // dump_tokens(scopes);
}