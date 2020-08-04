#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "grammar.h"
#include "onigmognu.h"
#include "parse.h"
#include "private.h"
#include "reader.h"

namespace parse {

grammar_t::grammar_t(Json::Value const& json)
{
    std::string scopeName = json["scopeName"].asString();
    _rule = add_grammar(scopeName, json);

    // Json::Value parsed = rule_to_json(_rule);
    // std::cout << parsed << std::endl;
    // doc = parsed;

    doc = json;
}

grammar_t::~grammar_t() {}

static bool pattern_has_back_reference(std::string const& ptrn)
{
    bool escape = false;
    for (char const& ch : ptrn) {
        if (escape && isdigit(ch)) {
            // D(DBF_Parser, bug("%s: %s\n", ptrn.c_str(), "YES"););
            return true;
        }
        escape = !escape && ch == '\\';
    }
    // D(DBF_Parser, bug("%s: %s\n", ptrn.c_str(), "NO"););
    return false;
}

static bool pattern_has_anchor(std::string const& ptrn)
{
    bool escape = false;
    for (char const& ch : ptrn) {
        if (escape && ch == 'G')
            return true;
        escape = !escape && ch == '\\';
    }
    return false;
}

// =============
// = grammar_t =
// =============

static void compile_patterns(rule_t* rule)
{
    if (rule->match_string != NULL_STR) {
        rule->match_pattern = regexp::pattern_t(rule->match_string);
        rule->match_pattern_is_anchored = pattern_has_anchor(rule->match_string);
        // if(!rule->match_pattern)
        //   os_log_error(OS_LOG_DEFAULT, "Bad begin/match pattern for %{public}s",
        //   rule->scope_string.c_str());
    }

    if (rule->while_string != NULL_STR && !pattern_has_back_reference(rule->while_string)) {
        rule->while_pattern = regexp::pattern_t(rule->while_string);
        // if(!rule->while_pattern)
        //   os_log_error(OS_LOG_DEFAULT, "Bad while pattern for %{public}s",
        //   rule->scope_string.c_str());
    }

    if (rule->end_string != NULL_STR && !pattern_has_back_reference(rule->end_string)) {
        rule->end_pattern = regexp::pattern_t(rule->end_string);
        // if(!rule->end_pattern)
        //   os_log_error(OS_LOG_DEFAULT, "Bad end pattern for %{public}s",
        //   rule->scope_string.c_str());
    }

    for (rule_ptr child : rule->children)
        compile_patterns(child.get());

    repository_ptr maps[] = { rule->repository, rule->injection_rules,
        rule->captures, rule->begin_captures,
        rule->while_captures, rule->end_captures };
    for (auto const& map : maps) {
        if (!map)
            continue;

        for (auto const& pair : *map)
            compile_patterns(pair.second.get());
    }

    // if (rule->injection_rules)
    //   std::copy(rule->injection_rules->begin(), rule->injection_rules->end(),
    //             back_inserter(rule->injections));
    // rule->injection_rules.reset();
}

void grammar_t::setup_includes(rule_ptr const& rule, rule_ptr const& base,
    rule_ptr const& self,
    rule_stack_t const& stack)
{

    std::string const include = rule->include_string;
    if (include == "$base") {
        rule->include = base.get();
    } else if (include == "$self") {
        rule->include = self.get();
    } else if (include != NULL_STR) {
        static auto find_repository_item = [](rule_t const* rule,
                                               std::string const& name) -> rule_t* {
            if (rule->repository) {
                auto it = rule->repository->find(name);
                if (it != rule->repository->end())
                    return it->second.get();
            }
            return nullptr;
        };

        if (include[0] == '#') {
            std::string const name = include.substr(1);
            for (rule_stack_t const* node = &stack; node && !rule->include;
                 node = node->parent)
                rule->include = find_repository_item(node->rule, name);
        } else {
            std::string::size_type fragment = include.find('#');
            if (rule_ptr grammar = find_grammar(include.substr(0, fragment), base))
                rule->include = fragment == std::string::npos
                    ? grammar.get()
                    : find_repository_item(
                        grammar.get(), include.substr(fragment + 1));
        }

        if (!rule->include) {
            // if (base != self)
            //   os_log_error(OS_LOG_DEFAULT,
            //                "%{public}s → %{public}s: include not found
            //                ‘%{public}s’", base->scope_string.c_str(),
            //                self->scope_string.c_str(), include.c_str());
            // else
            //   os_log_error(OS_LOG_DEFAULT,
            //                "%{public}s: include not found ‘%{public}s’",
            //                self->scope_string.c_str(), include.c_str());
        }
    } else {
        for (rule_ptr child : rule->children)
            setup_includes(child, base, self, rule_stack_t(rule.get(), &stack));

        repository_ptr maps[] = { rule->repository, rule->injection_rules,
            rule->captures, rule->begin_captures,
            rule->while_captures, rule->end_captures };
        for (auto const& map : maps) {
            if (!map)
                continue;

            for (auto const& pair : *map)
                setup_includes(pair.second, base, self,
                    rule_stack_t(rule.get(), &stack));
        }
    }
}

std::vector<std::pair<scope::selector_t, rule_ptr>>
grammar_t::injection_grammars()
{
    std::vector<std::pair<scope::selector_t, rule_ptr>> res;
    // TODO find grammar in directory
    // for(auto item : bundles::query(bundles::kFieldAny, NULL_STR,
    // scope::wildcard, bundles::kItemTypeGrammar))
    // {
    //   std::string injectionSelector =
    //   item->value_for_field(bundles::kFieldGrammarInjectionSelector);
    //   if(injectionSelector != NULL_STR)
    //   {
    //     if(rule_ptr grammar = convert_plist(item->plist()))
    //     {
    //       setup_includes(grammar, grammar, grammar,
    //       rule_stack_t(grammar.get())); compile_patterns(grammar.get());
    //       res.emplace_back(injectionSelector, grammar);
    //     }
    //   }
    // }
    return res;
}

rule_ptr grammar_t::find_grammar(std::string const& scope,
    rule_ptr const& base)
{
    auto it = _grammars.find(scope);
    if (it != _grammars.end())
        return it->second;
    // for(auto item : bundles::query(bundles::kFieldGrammarScope, scope,
    // scope::wildcard, bundles::kItemTypeGrammar))
    //   return add_grammar(scope, item->plist(), base);
    // return rule_ptr();
    return nullptr;
}

rule_ptr grammar_t::add_grammar(std::string const& scope,
    Json::Value const& json, rule_ptr const& base)
{

    rule_ptr grammar = convert_json(json);
    if (grammar) {
        _grammars.emplace(scope, grammar);
        setup_includes(grammar, base ?: grammar, grammar,
            rule_stack_t(grammar.get()));
        compile_patterns(grammar.get());
    }

    return grammar;
}

stack_ptr grammar_t::seed() const
{
    return std::make_shared<stack_t>(_rule.get(),
        _rule ? _rule->scope_string : "");
}

grammar_ptr parse_grammar(Json::Value const& json)
{
    return std::make_shared<grammar_t>(json);
}

} // namespace parse