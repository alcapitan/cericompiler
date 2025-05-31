#include <string>
using namespace std;

// j'ai tout commenté ici pour bénéficier des fonctionnalités d'aide docstring de mon IDE

/*
Types de tokens utilisés par l'analyseur lexical.
Chaque token représente un symbole dans le langage.
*/
enum TOKEN
{
    FEOF,        // Fin de fichier
    UNKNOWN,     // Token inconnu
    BOOLVAL,     // Booléen
    VARDECL,     // Mot clé pour déclaration de variable
    VARTYPE,     // Type de variable
    KEYWORD,     // Mot clé pour une instruction
    NUMBER,      // Nombre
    ID,          // Variable
    STRINGCONST, // String
    RBRACKET,    // Crochet gauche
    LBRACKET,    // Crochet droite
    RPARENT,     // Parenthèse gauche
    LPARENT,     // Parenthèse droite
    COMMA,       // Virgule
    SEMICOLON,   // Point-virgule
    EXIT,        // Fin de programme
    ADDOP,       // Opérateur additif
    MULOP,       // Opérateur multiplicatif
    RELOP,       // Opérateur de comparaison
    NOT,         // Négation
    ASSIGN       // Assignation de valeur à une variable
};

/*
Permet de print le type de token, utile dans les messages d'erreur

Fonction désactivée car fait boguer l'analyseur lexical

string to_string(TOKEN token)
{
    switch (token)
    {
    case FEOF:
        return "Fin de fichier";
    case BOOLVAL:
        return "Booléen";
    case VARDECL:
        return "Mot clé pour déclaration de variable";
    case VARTYPE:
        return "Type de variable";
    case KEYWORD:
        return "Mot clé pour une instruction";
    case NUMBER:
        return "Nombre";
    case ID:
        return "Variable";
    case STRINGCONST:
        return "String";
    case RBRACKET:
        return "Crochet gauche";
    case LBRACKET:
        return "Crochet droite";
    case RPARENT:
        return "Parenthèse gauche";
    case LPARENT:
        return "Parenthèse droite";
    case COMMA:
        return "Virgule";
    case SEMICOLON:
        return "Point-virgule";
    case EXIT:
        return "Fin de programme";
    case ADDOP:
        return "Opérateur additif";
    case MULOP:
        return "Opérateur multiplicatif";
    case RELOP:
        return "Opérateur de comparaison";
    case NOT:
        return "Négation";
    case ASSIGN:
        return "Assignation de valeur à une variable";
    default:
        return "Token inconnu";
    }
}*/