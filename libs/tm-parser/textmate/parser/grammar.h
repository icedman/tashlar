#ifndef PARSE_GRAMMAR_H
#define PARSE_GRAMMAR_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "json/json.h"
#include "private.h"
#include "scope.h"

namespace parse {

struct rule_t;
struct stack_t;
typedef std::shared_ptr<rule_t> rule_ptr;
typedef std::shared_ptr<stack_t> stack_ptr;

struct grammar_t {

    grammar_t(Json::Value const& json);
    ~grammar_t();

    stack_ptr seed() const;
    std::mutex& mutex() { return _mutex; }
    Json::Value document() { return doc; }

private:
    struct rule_stack_t {
        rule_stack_t(rule_t const* rule, rule_stack_t const* parent = nullptr)
            : rule(rule)
            , parent(parent)
        {
        }

        rule_t const* rule;
        rule_stack_t const* parent;
    };

    void setup_includes(rule_ptr const& rule, rule_ptr const& base,
        rule_ptr const& self, rule_stack_t const& stack);
    rule_ptr find_grammar(std::string const& scope, rule_ptr const& base);

    std::vector<std::pair<scope::selector_t, rule_ptr>> injection_grammars();

    rule_ptr add_grammar(std::string const& scope, Json::Value const& json,
        rule_ptr const& base = rule_ptr());
    rule_ptr _rule;
    std::map<std::string, rule_ptr> _grammars;
    std::mutex _mutex;

    Json::Value doc;
};

typedef std::shared_ptr<grammar_t> grammar_ptr;
grammar_ptr parse_grammar(Json::Value const& json);

} // namespace parse

#endif