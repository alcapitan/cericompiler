// tokeniser.h : shared definition for tokeniser.l and compilateur.cpp

enum TOKEN
{
    FEOF,
    UNKNOWN,
    BOOLVAL,
    VARDECL,
    VARTYPE,
    KEYWORD,
    NUMBER,
    ID,
    STRINGCONST,
    RBRACKET,
    LBRACKET,
    RPARENT,
    LPARENT,
    COMMA,
    SEMICOLON,
    EXIT,
    ADDOP,
    MULOP,
    RELOP,
    NOT,
    ASSIGN
};
