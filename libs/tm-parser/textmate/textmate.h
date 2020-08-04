#ifndef TEXTMATE_H
#define TEXTMATE_H

struct textmate_t {
    struct tm_grammar_t* grammar;
    struct tm_theme_t* theme;
    struct tm_state_t* state;
};

#ifndef IMPORT_TEXTMATE
extern "C" {
#endif

void tm_init(struct textmate_t* tm);
void tm_free(struct textmate_t* tm);
void tm_load_grammar(struct textmate_t* tm, char* filename);
void tm_load_theme(struct textmate_t* tm, char* filename);
void tm_parse_line(struct textmate_t* tm, char const* first, char const* last);

#ifndef IMPORT_TEXTMATE
}
#endif

#endif // TEXTMATE_H
