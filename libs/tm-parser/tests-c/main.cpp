#include "textmate.h"

#include <cstring>
int main(int argc, char** argv)
{
    struct textmate_t tm;

    tm_init(&tm);
    tm_load_grammar(&tm, "extensions/cpp/syntaxes/c.tmLanguage.json");
    tm_load_theme(&tm, "test-cases/themes/light_vs.json");

    char *test = "int main(int argc, char** argv)";
    char *first = test;
    char *last = first + strlen(test);

    tm_parse_line(&tm, first, last);

    tm_free(&tm);

    return 0;
}