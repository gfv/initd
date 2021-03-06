#ifndef NODE_TYPE_H
#define NODE_TYPE_H

enum class token_type
{
    unknown,
    eof,
    error,
    whitespace,
    identifier,
    integer_literal,
    string_literal,
    lbrace,
    rbrace,
    equals,
    semicolon,
    comma,
    colon,
    task_keyword,
    true_keyword,
    false_keyword,
    depends_keyword,
    run_level_keyword,
};

#endif
