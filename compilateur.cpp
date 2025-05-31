#include <iostream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <FlexLexer.h>
#include "tokeniser.h"
using namespace std;

/*
Types de variables possibles
*/
enum VariableType
{
    INT,  // Entier 64 bits
    BOOL, // Booléen
};

/*
Permet de print le type de variable, utile dans les messages d'erreur
*/
string to_string(VariableType type)
{
    switch (type)
    {
    case VariableType::INT:
        return "INT";
    case VariableType::BOOL:
        return "BOOL";
    default:
        return "UNKNOWN";
    }
}

/*
Les propriétés d'une variable qui seront utilisées pour les vérifications de type
*/
struct VariableProperties
{
    VariableType type; // Type de la variable
    bool isAssigned;   // Si la variable a été assignée
    bool isConst;      // Si la variable est constante
};

/*
Liste des variables déclarées
*/
unordered_map<string, VariableProperties> variablesDeclarees;

int jmpId = 0;                      // Identifiant de saut pour distinguer les boucles et conditions entre elles et éviter les conflits
FlexLexer *lexer = new yyFlexLexer; // Analyseur lexical pour lire les tokens
TOKEN currentToken;                 // Pointeur de lecture de l'analyseur lexical

/*
Déplace le pointeur de lecture sur le prochain token.
Sert à éclaircir le code.
*/
void TokenSuivant()
{
    currentToken = (TOKEN)lexer->yylex();
}

/*
Affiche un message d’erreur en rouge dans stderr et arrête le programme.
*/
void ThrowError(string message)
{
    cerr << "\033[31m"; // Texte en rouge
    cerr << "ERREUR : " << message << endl;
    cerr << "\tPosition : ligne " << lexer->lineno() << endl;
    cerr << "\tCaractères lu : " << lexer->YYText() << " (type " << currentToken << ")" << endl;
    cerr << "\033[0m"; // Reset couleur du texte
    exit(-1);
}

/*
Affiche un message d'alerte en jaune dans stderr, mais ne stoppe pas le programme car l'erreur signalé ne compromet pas le programme.
*/
void ThrowWarning(string message)
{
    cerr << "\033[33m"; // Texte en jaune
    cerr << "ATTENTION : " << message << endl;
    cerr << "\tPosition : ligne " << lexer->lineno() << endl;
    cerr << "\tCaractères lus : " << lexer->YYText() << " (type " << currentToken << ")" << endl;
    cerr << "\033[0m"; // Reset couleur du texte
}

/*
Affiche un message de débogage en bleu dans stderr, utile pour suivre le déroulement du programme pour déboguer.
*/
void PrintDebug(string message)
{
    cerr << "\033[34m"; // Texte en bleu
    cerr << "\t# " << message << endl;
    cerr << "\033[0m"; // Reset couleur du texte
}

/*
Lit un nom de variable, vérifie s'il est déclaré et assigné, et complète le code assembleur.
*/
VariableType Variable()
{
    PrintDebug("Variable()");

    string nomVariable = lexer->YYText();
    VariableType typeVariable = variablesDeclarees[nomVariable].type;

    if (variablesDeclarees.find(lexer->YYText()) == variablesDeclarees.end())
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
    else if (variablesDeclarees[lexer->YYText()].isAssigned == false)
        ThrowError("Variable non assignée : " + string(lexer->YYText()));

    cout << "\tpush " << lexer->YYText() << endl;
    TokenSuivant();

    return typeVariable;
}

/*
Lit un nombre entier et complète le code assembleur.
*/
VariableType NombreEntier()
{
    PrintDebug("NombreEntier()");

    // il ne peut gérer que des entiers 32 bits mais je n'arrive pas à faire une condition anti overflow
    cout << "\tpush $" << atoi(lexer->YYText()) << endl;
    TokenSuivant();

    return INT;
}

/*
Lit une valeur booléenne et complète le code assembleur.
- True : -1
- False : 0
*/
VariableType Booleen()
{
    PrintDebug("Booleen()");

    if (strcmp(lexer->YYText(), "True") == 0)
        cout << "\tpush $-1\t\t# True" << endl;
    else if (strcmp(lexer->YYText(), "False") == 0)
        cout << "\tpush $0\t\t# False" << endl;

    TokenSuivant();

    return BOOL;
}

VariableType Expression(); // Déclaration abstracte car interdépendance avec Facteur()

/*
Interprète un facteur, c'est-à-dire une opérande dans une opération multiplicative (Terme)
Gère les expressions entre parenthèses, les nombres, les booléens et les variables.
*/
VariableType Facteur()
{
    PrintDebug("Facteur()");
    VariableType typeFacteur;
    if (currentToken == RPARENT)
    {
        TokenSuivant();
        typeFacteur = Expression();
        if (currentToken != LPARENT)
            ThrowError("Parenthèse droite attendue");
        TokenSuivant();
    }
    else if (currentToken == NUMBER)
        typeFacteur = NombreEntier();
    else if (currentToken == BOOLVAL)
        typeFacteur = Booleen();
    else if (currentToken == ID)
        typeFacteur = Variable();
    else
        ThrowError("Parenthèse gauche ou nombre ou booléen ou variable attendue");

    return typeFacteur;
}

/*
Opérateurs multiplicatifs
*/
enum OPMUL
{
    MUL, // Multiplication
    DIV, // Division
    MOD, // Modulo
    AND, // ET logique
    WTFM // Erreur
};

/*
Permet de print l'opérateur multiplicatif, utile dans les messages d'erreur
*/
string to_string(OPMUL op)
{
    switch (op)
    {
    case MUL:
        return "MUL";
    case DIV:
        return "DIV";
    case MOD:
        return "MOD";
    case AND:
        return "AND";
    default:
        return "WTFM";
    }
}

/*
Retourne l'identifiant de l'opérateur multiplicatif rencontré.
*/
OPMUL OperateurMultiplicatif()
{
    PrintDebug("OperateurMultiplicatif()");
    OPMUL opmul;
    if (strcmp(lexer->YYText(), "*") == 0)
        opmul = MUL;
    else if (strcmp(lexer->YYText(), "/") == 0)
        opmul = DIV;
    else if (strcmp(lexer->YYText(), "%") == 0)
        opmul = MOD;
    else if (strcmp(lexer->YYText(), "&&") == 0)
        opmul = AND;
    else
        opmul = WTFM;
    TokenSuivant();
    return opmul;
}

/*
Lit un terme, c'est-à-dire une expression multiplicative.
*/
VariableType Terme()
{
    PrintDebug("Terme()");
    OPMUL mulop;
    VariableType typeFacteur1, typeFacteur2;
    typeFacteur1 = Facteur();
    // si le calcul se poursuit, ou se termine
    while (currentToken == MULOP)
    {
        mulop = OperateurMultiplicatif();
        PrintDebug("Opérateur multiplicatif : " + to_string(mulop));
        typeFacteur2 = Facteur();

        // vérification des types
        if (typeFacteur1 != typeFacteur2)
            ThrowError("Impossible de calculer, les opérandes n'ont pas le même type");

        cout << "\tpop	%rbx" << endl; // premier opérande
        cout << "\tpop	%rax" << endl; // second opérande

        switch (mulop)
        {
        case AND:
            if (typeFacteur1 == INT)
                ThrowError("Opération illogique entre nombres " + to_string(mulop));
            cout << "\tandq	%rbx, %rax" << endl;
            cout << "\tpush %rax" << endl;
            break;
        case MUL:
            if (typeFacteur1 == BOOL)
                ThrowError("Opération illogique entre booléens" + to_string(mulop));
            cout << "\timul %rbx, %rax" << endl;
            cout << "\tpush %rax" << endl;
            break;
        case DIV:
            if (typeFacteur1 == BOOL)
                ThrowError("Opération illogique entre booléens" + to_string(mulop));
            cout << "\tmovq $0, %rdx" << endl; // reset registre de reste de division
            cout << "\tidiv %rbx, %rax" << endl;
            cout << "\tpush %rax" << endl;
            break;
        case MOD:
            if (typeFacteur1 == BOOL)
                ThrowError("Opération illogique entre booléens" + to_string(mulop));
            cout << "\tmovq $0, %rdx" << endl; // reset registre de reste de division
            cout << "\tidiv %rbx, %rax" << endl;
            cout << "\tpush %rdx" << endl; // le résultat du modulo est le reste de la division
            break;
        default:
            ThrowError("Opérateur multiplicatif inattendu : " + to_string(mulop));
            break;
        }
    }

    return typeFacteur1;
}

/*
Opérateurs arithmétiques
*/
enum OPADD
{
    ADD, // Addition
    SUB, // Soustraction
    OR,  // OU logique
    WTFA // Opérateur inconnu
};

/*
Permet de print l'opérateur arithmétique, utile dans les messages d'erreur
*/
string to_string(OPADD op)
{
    switch (op)
    {
    case ADD:
        return "ADD";
    case SUB:
        return "SUB";
    case OR:
        return "OR";
    default:
        return "WTFA";
    }
}

/*
Retourne l'identifiant de l'opérateur additif rencontré.
*/
OPADD OperateurAdditif()
{
    PrintDebug("OperateurAdditif()");
    OPADD opadd;
    if (strcmp(lexer->YYText(), "+") == 0)
        opadd = ADD;
    else if (strcmp(lexer->YYText(), "-") == 0)
        opadd = SUB;
    else if (strcmp(lexer->YYText(), "||") == 0)
        opadd = OR;
    else
        opadd = WTFA;
    TokenSuivant();
    return opadd;
}

/*
Lit une expression mathématique, c'est-à-dire une addition ou une soustraction de termes.
*/
VariableType ExpressionMathematique()
{
    PrintDebug("ExpressionMathematique()");
    OPADD adop; // ADditive OPerator
    VariableType typeTerm1, typeTerm2;
    typeTerm1 = Terme();
    // si le calcul se poursuit, ou se termine
    while (currentToken == ADDOP)
    {
        adop = OperateurAdditif();
        PrintDebug("Opérateur additif : " + to_string(adop));
        typeTerm2 = Terme();

        // vérification des types
        if (typeTerm1 != typeTerm2)
            ThrowError("Impossible de calculer, les opérandes n'ont pas le même type (type 1 : " + to_string(typeTerm1) + ", type 2 : " + to_string(typeTerm2) + ")");

        cout << "\tpop	%rbx" << endl; // premier opérande
        cout << "\tpop	%rax" << endl; // second opérande
        switch (adop)
        {
        case OR:
            if (typeTerm1 == INT)
                ThrowError("Opération illogique entre nombres " + to_string(adop));
            cout << "\taddq	%rbx, %rax" << endl;
            break;
        case ADD:
            if (typeTerm1 == BOOL)
                ThrowError("Opération illogique entre booléens " + to_string(adop));
            cout << "\taddq	%rbx, %rax" << endl;
            break;
        case SUB:
            if (typeTerm1 == BOOL)
                ThrowError("Opération illogique entre booléens " + to_string(adop));
            cout << "\tsubq	%rbx, %rax" << endl;
            break;
        default:
            ThrowError("Opérateur additif inattendu : " + to_string(adop));
            break;
        }
        cout << "\tpush	%rax" << endl;
    }

    return typeTerm1;
}

/*
Opérateurs de comparaison
*/
enum OPREL
{
    EQU,  // Égal
    DIFF, // Non égal
    INF,  // Inférieur
    SUP,  // Supérieur
    INFE, // Inférieur ou égal
    SUPE, // Supérieur ou égal
    WTFR  // Opérateur inconnu
};

/*
Permet de print l'opérateur de comparaison, utile dans les messages d'erreur
*/
string to_string(OPREL op)
{
    switch (op)
    {
    case EQU:
        return "==";
    case DIFF:
        return "!=";
    case INF:
        return "<";
    case SUP:
        return ">";
    case INFE:
        return "<=";
    case SUPE:
        return ">=";
    default:
        return "WTFR";
    }
}

/*
Retourne l'identifiant de l'opérateur de comparaison rencontré.
*/
OPREL OperateurComparaison()
{
    PrintDebug("OperateurComparaison()");
    OPREL oprel;
    if (strcmp(lexer->YYText(), "==") == 0)
        oprel = EQU;
    else if (strcmp(lexer->YYText(), "!=") == 0)
        oprel = DIFF;
    else if (strcmp(lexer->YYText(), "<") == 0)
        oprel = INF;
    else if (strcmp(lexer->YYText(), ">") == 0)
        oprel = SUP;
    else if (strcmp(lexer->YYText(), "<=") == 0)
        oprel = INFE;
    else if (strcmp(lexer->YYText(), ">=") == 0)
        oprel = SUPE;
    else
        oprel = WTFR;
    TokenSuivant();
    return oprel;
}

/*
Lit une expression, c'est une expression de comparaison s'il y a un opérateur associé, sinon c'est une expression mathématique.
*/
VariableType Expression()
{
    PrintDebug("Expression()");
    OPREL oprel;
    VariableType typeExpr1, typeExpr2;
    typeExpr1 = ExpressionMathematique();
    if (currentToken == RELOP)
    {
        oprel = OperateurComparaison();
        PrintDebug("Opérateur de comparaison : " + to_string(oprel));
        typeExpr2 = ExpressionMathematique();

        // vérification des types
        if (typeExpr1 != typeExpr2)
            ThrowError("Impossible de comparer, les opérandes n'ont pas le même type (type 1 : " + to_string(typeExpr1) + ", type 2 : " + to_string(typeExpr2) + ")");

        cout << "\tpop	%rax" << endl; // seconde opérande
        cout << "\tpop	%rbx" << endl; // première opérande
        cout << "\tcmp	%rax, %rbx" << endl;
        switch (oprel)
        {
        case INF:
            cout << "\tjl	.truecmp" << jmpId << endl;
            break;
        case INFE:
            cout << "\tjle	.truecmp" << jmpId << endl;
            break;
        case SUP:
            cout << "\tjg	.truecmp" << jmpId << endl;
            break;
        case SUPE:
            cout << "\tjge	.truecmp" << jmpId << endl;
            break;
        case EQU:
            cout << "\tje	.truecmp" << jmpId << endl;
            break;
        case DIFF:
            cout << "\tjne	.truecmp" << jmpId << endl;
            break;
        default:
            ThrowError("Opérateur de comparaison inattendu : " + to_string(oprel));
            break;
        }
        cout << "\tpush $0\t\t# False" << endl; // false
        cout << "\tjmp	.endcmp" << jmpId << endl;
        cout << ".truecmp" << jmpId << ":" << endl;
        cout << "\tpush $-1\t\t# True" << endl; // true
        cout << ".endcmp" << jmpId << ":" << endl;
        jmpId++;

        return BOOL; // le résultat d'une comparaison est toujours un booléen
    }

    return typeExpr1; // si pas de comparaison, on retourne le type de l'expression
}

/*
Lit une instruction d'assignation, vérifie la validité de l'instruction, et complète le code assembleur.
*/
void InstructionAssignation()
{
    PrintDebug("InstructionAssignation()");

    // lecture du nom de la variable
    string nomVariable;
    if (currentToken != ID)
        ThrowError("Nom de variable attendu");
    nomVariable = lexer->YYText();
    PrintDebug("Assignation de la variable : " + nomVariable);

    // vérification de la variable
    if (variablesDeclarees.find(nomVariable) == variablesDeclarees.end())
        ThrowError("Variable non déclarée : " + string(nomVariable));
    else if (variablesDeclarees[nomVariable].isConst == true && variablesDeclarees[nomVariable].isAssigned == true)
        ThrowError("Variable constante déjà assigné : " + string(nomVariable));
    variablesDeclarees[nomVariable].isAssigned = true;

    // lecture de l'assignation
    TokenSuivant();
    if (currentToken != ASSIGN)
        ThrowError("Opérateur ':=' attendu");
    else
        TokenSuivant();
    VariableType typeExpr = Expression();

    // vérification des types entre la variable et l'expression
    if (variablesDeclarees[nomVariable].type != typeExpr)
        ThrowError("Impossible d'assigner, les types ne correspondent pas (variable : " + to_string(variablesDeclarees[nomVariable].type) + ", expression : " + to_string(typeExpr) + ")");

    cout << "\tpop " << nomVariable << endl;

    if (currentToken != SEMICOLON)
        ThrowError("Fin instruction inattendue, ';' attendu");
}

/*
Affiche une expression dans le terminal.
*/
void InstructionPrint()
{
    PrintDebug("InstructionPrint()");
    TokenSuivant();
    VariableType typeExpr;
    typeExpr = Expression(); // on lit l'expression à afficher
    switch (typeExpr)
    {
    case INT:
        cout << "\tpop %rsi\t# The value to be displayed" << endl;
        cout << "\tmovq $FormatPrintInt, %rdi" << endl;
        cout << "\tmovl $0, %eax" << endl;
        cout << "\tcall printf@PLT" << endl;
        break;
    case BOOL:
        cout << "\tpop %rdx\t# Zero : False, non-zero : true" << endl;
        cout << "\tcmpq $0, %rdx" << endl;
        cout << "\tje .printfalse" << jmpId << endl;
        cout << "\tmovq $FormatPrintTrue, %rdi" << endl;
        cout << "\tjmp .endprint" << jmpId << endl;
        cout << ".printfalse" << jmpId << ":" << endl;
        cout << "\tmovq $FormatPrintFalse, %rdi" << endl;
        cout << ".endprint" << jmpId << ":" << endl;
        cout << "\tcall	puts@PLT" << endl;
        jmpId++;
        break;
    default:
        ThrowError("Type d'expression non géré pour l'instruction PRINT : " + to_string(typeExpr));
    }

    if (currentToken != SEMICOLON)
        ThrowError("Fin d'instruction inattendue, ';' attendu");
}

void Instruction(); // Déclaration abstracte car interdépendance avec BlocFor(), BlocWhile() et IfStatement()

/*
Lit un bloc FOR, vérifie la validité de l'instruction de boucle, et complète le code assembleur.
*/
void BlocFor()
{
    PrintDebug("BlocFor()");

    // mot clé FOR
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "FOR") != 0)
        ThrowError("Mot clé 'FOR' attendu");
    TokenSuivant();

    // variable de boucle
    if (currentToken != ID)
        ThrowError("Nom de variable attendu");
    string nomVariableBoucle = lexer->YYText();
    if (variablesDeclarees.find(lexer->YYText()) == variablesDeclarees.end())
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
    else if (variablesDeclarees[nomVariableBoucle].isAssigned == true)
        ThrowWarning("Variable boucle FOR déjà assigné hors de la boucle, risque de perte de données : " + string(lexer->YYText()));
    variablesDeclarees[nomVariableBoucle].isAssigned = true; // on déclare la variable comme utilisée
    TokenSuivant();

    // opérateur d'assignation de la variable de boucle
    if (currentToken != ASSIGN)
        ThrowError("Opérateur ':=' attendu");
    TokenSuivant();

    // expression d'initialisation de la variable de boucle
    ExpressionMathematique(); // valeur de départ de la variable de boucle
    cout << "\tpop " << nomVariableBoucle << endl;
    cout << "\tmovq " << nomVariableBoucle << ", %rax" << endl;

    // mot clé TO ou DOWNTO
    bool loopIncrement = true; // TO = true, DOWNTO = false
    if (currentToken == KEYWORD)
    {
        if (strcmp(lexer->YYText(), "TO") == 0)
            loopIncrement = true; // boucle croissante
        else if (strcmp(lexer->YYText(), "DOWNTO") == 0)
            loopIncrement = false; // boucle décroissante
        else
            ThrowError("Mot clé 'TO' ou 'DOWNTO' attendu");
    }
    else
        ThrowError("Mot clé 'TO' ou 'DOWNTO' attendu");
    TokenSuivant();

    // expression de fin de boucle
    ExpressionMathematique(); // valeur de fin de la variable de boucle

    // début et comparaison de la boucle en assembleur
    cout << "\tpop %rbx" << endl;
    int loopJmpId = jmpId;                      // on sauvegarde l'ID de la boucle pour pouvoir le réutiliser même quand il sera incrémenté par ses propres instructions
    jmpId++;                                    // on l'incrémente maintenant pour la différencier de ses propres instructions
    cout << ".for" << loopJmpId << ":" << endl; // début de la boucle
    cout << "\tcmp %rbx, %rax" << endl;
    // si la valeur de boucle est dépasse la valeur de fin, on sort de la boucle
    if (loopIncrement)
        cout << "\tjg .endfor" << loopJmpId << endl;
    else
        cout << "\tjl .endfor" << loopJmpId << endl;

    cout << "\tpush " << nomVariableBoucle << endl; // on empile la variable de boucle pour pouvoir l'utiliser dans l'instruction increment/decrément

    // mot clé DO
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "DO") != 0)
        ThrowError("'DO' attendu");
    TokenSuivant();

    // toutes les instructions dans la boucle (entre DO et ENDFOR)
    while (strcmp(lexer->YYText(), "ENDFOR") != 0)
        Instruction();

    // incrémentation de la variable de boucle puis remontée au début de la boucle pour comparaison
    cout << "\tpop " << nomVariableBoucle << endl; // on dépile la variable de boucle pour l'utiliser dans l'instruction d'incrément/decrément
    if (loopIncrement)
        cout << "\taddq $1, " << nomVariableBoucle << endl; // on incrémente la variable de boucle
    else
        cout << "\tsubq $1, " << nomVariableBoucle << endl;     // on décrémente la variable de boucle
    cout << "\tmovq " << nomVariableBoucle << ", %rax" << endl; // on met à jour la variable de boucle dans le registre
    cout << "\tjmp .for" << loopJmpId << endl;                  // on revient au début de la boucle pour revérifier la condition
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "ENDFOR") != 0)
        ThrowError("'ENDFOR' attendu");
    cout << ".endfor" << loopJmpId << ":" << endl;
}

/*
Lit un bloc WHILE, vérifie la validité de l'instruction de boucle, et complète le code assembleur.
*/
void BlocWhile()
{
    PrintDebug("BlocWhile()");

    // mot clé WHILE
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "WHILE") != 0)
        ThrowError("'WHILE' attendu");
    TokenSuivant();

    // début et condition de la boucle en assembleur
    int loopJmpId = jmpId;                        // on sauvegarde l'ID de la boucle pour pouvoir le réutiliser même quand il sera incrémenté par ses propres instructions
    jmpId++;                                      // on l'incrémente maintenant pour la différencier de ses propres instructions
    cout << ".while" << loopJmpId << ":" << endl; // début de la boucle
    Expression();                                 // expression de la condition
    cout << "\tpop	%rax" << endl;
    cout << "\tcmp	$0, %rax" << endl; // si l'expression est fausse (égal à 0), on sort de la boucle
    cout << "\tje	.endwhile" << loopJmpId << endl;

    // mot clé DO
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "DO") != 0)
        ThrowError("'DO' attendu");
    TokenSuivant();

    // toutes les instructions dans la boucle (entre DO et ENDWHILE)
    while (strcmp(lexer->YYText(), "ENDWHILE") != 0)
        Instruction();

    // on remonte au début de la boucle pour revérifier la condition
    cout << "\tjmp	.while" << loopJmpId << endl;
    cout << ".endwhile" << loopJmpId << ":" << endl;
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "ENDWHILE") != 0)
        ThrowError("'ENDWHILE' attendu");
}

/*
Lit une instruction IF, vérifie la validité de l'instruction, et complète le code assembleur.
*/
void IfStatement()
{
    PrintDebug("IfStatement()");

    // mot clé IF
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "IF") != 0)
        ThrowError("'IF' attendu");
    TokenSuivant();

    // expression de la condition
    Expression();

    // mot clé THEN
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "THEN") != 0)
        ThrowError("'THEN' attendu");
    TokenSuivant();

    // Si l'expression est fausse, on saute à la partie ELSE
    int blocJmpId = jmpId; // on sauvegarde l'ID du bloc pour pouvoir le réutiliser même quand il sera incrémenté par ses propres instructions
    jmpId++;
    cout << "\tpop	%rax" << endl; // on récupère le résultat de l'expression
    cout << "\tcmp	$0, %rax" << endl;
    cout << "\tje	.else" << blocJmpId << endl;

    // les instructions dans le bloc THEN (se terminent par un ENDIF ou ELSE)
    while (strcmp(lexer->YYText(), "ENDIF") != 0 && strcmp(lexer->YYText(), "ELSE") != 0)
        Instruction();

    // après THEN, on saute à la fin de l'IF pour ne pas exécuter le bloc ELSE
    cout << "\tjmp	.endif" << blocJmpId << endl;

    // bloc ELSE
    cout << ".else" << blocJmpId << ":" << endl; // on écrit toujours ce label car il est mentionné en début de boucle
    // si il y a un bloc ELSE
    if (currentToken == KEYWORD && strcmp(lexer->YYText(), "ELSE") == 0)
    {
        TokenSuivant();
        // toutes les instructions dans le bloc ELSE (se terminent par un ENDIF)
        while (strcmp(lexer->YYText(), "ENDIF") != 0)
            Instruction();
    }

    // mot clé ENDIF
    if (currentToken != KEYWORD || strcmp(lexer->YYText(), "ENDIF") != 0)
        ThrowError("'ENDIF' attendu");
    cout << ".endif" << blocJmpId << ":" << endl; // on sort de l'IF
}

/*
Redirige la lecture de l'instruction vers la fonction appropriée selon son mot-clé.
*/
void Instruction()
{
    PrintDebug("Instruction()");

    if (currentToken == SEMICOLON)
        ThrowWarning("Instruction vide");
    else if (currentToken == KEYWORD)
    {
        if (strcmp(lexer->YYText(), "IF") == 0)
            IfStatement();
        else if (strcmp(lexer->YYText(), "WHILE") == 0)
            BlocWhile();
        else if (strcmp(lexer->YYText(), "FOR") == 0)
            BlocFor();
        else if (strcmp(lexer->YYText(), "PRINT") == 0)
            InstructionPrint();
        else
            ThrowError("Instruction inconnue : " + string(lexer->YYText()));
    }
    else if (currentToken == ID)
        InstructionAssignation();
    else if (currentToken == VARDECL)
        ThrowError("Les variables doivent être déclarées avant toute instruction");
    else
        ThrowError("Instruction inconnue : " + string(lexer->YYText()));

    TokenSuivant(); // on sort de l'instruction
}

/*
Le début du programme est reservé à la déclaration des variables.
*/
void PartieDeclarationVariables()
{
    PrintDebug("PartieDeclarationVariables()");

    // Début obligatoire
    cout << ".data" << endl;
    cout << "\t.align 8" << endl;
    // variables globales pour les PRINT
    cout << "\tFormatPrintInt: .string \"%lld\\n\"\t# print 64-bit signed integers" << endl;
    cout << "\tFormatPrintTrue: .string \"True\"\t# print boolean true" << endl;
    cout << "\tFormatPrintFalse: .string \"False\"\t# print boolean false" << endl;

    // dans le cas où il n'y a pas de variables à déclarer
    if (currentToken != VARDECL)
        return;

    // on boucle tant qu'il y a des déclarations de variables
    while (currentToken == VARDECL)
    {
        // Instruction DECLARE obligatoire
        if (strcmp(lexer->YYText(), "DECLARE") != 0)
            ThrowError("Une déclaration de variable doit commencer par 'DECLARE'");

        // Variable constante ou non
        TokenSuivant();
        bool isConst = false;
        if (currentToken == VARDECL && strcmp(lexer->YYText(), "CONST") == 0)
        {
            isConst = true;
            TokenSuivant();
            PrintDebug("Variable constante");
        }
        else
            PrintDebug("Variable non constante");

        // Type de variable
        if (currentToken != VARTYPE)
            ThrowError("Type de variable manquant après 'DECLARE'");
        VariableType typeVariable;
        if (strcmp(lexer->YYText(), "INT") == 0)
            typeVariable = INT;
        else if (strcmp(lexer->YYText(), "BOOL") == 0)
            typeVariable = BOOL;
        else
            ThrowError("Type de variable inconnu : " + string(lexer->YYText()));
        PrintDebug("Type de variable : " + string(lexer->YYText()));
        TokenSuivant();

        // Nom de variable
        if (currentToken != ID)
            ThrowError("Nom de variable manquant après le type de variable");
        string nomVariable = lexer->YYText();
        TokenSuivant();

        // Si assignation dès la déclaration
        if (currentToken == ASSIGN)
        {
            // On saute l'opérateur d'assignation
            TokenSuivant();

            // On lit la valeur d'assignation
            // Attention pas d'expressions autorisées dans la déclaration de variable, car les fonctions associées ne correspondent pas à ce qui est attendu dans .data
            if (typeVariable == INT)
            {
                if (currentToken != NUMBER)
                    ThrowError("Les expressions ne sont pas autorisées dans la déclaration de variable");
                cout << "\t" << nomVariable << ":\t" << ".quad " << lexer->YYText() << endl;
            }
            else if (typeVariable == BOOL)
            {
                if (currentToken != BOOLVAL)
                    ThrowError("Valeur booléenne attendue pour la variable constante");
                if (strcmp(lexer->YYText(), "True") == 0)
                    cout << "\t" << nomVariable << ":\t" << ".quad -1\t\t# True" << endl;
                else if (strcmp(lexer->YYText(), "False") == 0)
                    cout << "\t" << nomVariable << ":\t" << ".quad 0\t\t# False" << endl;
            }
            else
                ThrowError("Type de variable inconnu : " + string(lexer->YYText()));

            // Enregistrement de la variable dans la liste des variables déclarées
            variablesDeclarees[nomVariable] = {typeVariable, true, isConst}; // type de la variable, si elle est assignée, si elle est constante

            // fin d'assignation, on avance au point-virgule
            TokenSuivant();
        }
        else
        {
            // Si pas d'assignation, on initialise avec une valeur par défaut, mais elle ne doit pas être utilisée avant d'être assignée dans les instructions
            if (typeVariable == INT)
                cout << "\t" << nomVariable << ":\t" << ".quad 0\t\t# Initialisation à 0" << endl;
            else if (typeVariable == BOOL)
                cout << "\t" << nomVariable << ":\t" << ".quad 0\t\t# Initialisation à False" << endl;
            else
                ThrowError("Type de variable inconnu : " + string(lexer->YYText()));

            // Enregistrement de la variable dans la liste des variables déclarées
            variablesDeclarees[nomVariable] = {typeVariable, false, isConst}; // type de la variable, si elle est assignée, si elle est constante

            // quand il n'y a pas d'assignation, on n'a pas à avancer le pointeur de lecture car ça déjà naturellement été déjà fait
        }

        // Point-virgule à la fin de la déclaration comme pour les instructions
        if (currentToken != SEMICOLON)
            ThrowError("Fin de déclaration de variable manquante, ';' attendu");
        if (currentToken == VARDECL)
        {
            PrintDebug("Prochaine déclaration de variable");
        }

        TokenSuivant(); // on sort de la partie déclaration variables
    }
}

/*
Après la déclaration des variables, on entre dans la partie algorithme du programme.
Ici sont réalisées toutes les instructions du programme.
*/
void PartieAlgorithme()
{
    PrintDebug("PartieAlgorithme()");

    // Début obligatoire
    cout << ".text\t\t# The following lines contain the program" << endl;
    cout << "\t.globl main\t\t# The main function must be visible from outside" << endl;
    cout << "main:" << endl;
    cout << "\tmovq %rsp, %rbp\t\t# Save the position of the stack's top" << endl;

    // Dans le cas où il n'y a pas d'instructions à exécuter
    if (currentToken == EXIT)
        ThrowWarning("Aucune instruction trouvée, programme vide");

    // On boucle tant qu'il y a des instructions à exécuter
    while (currentToken != EXIT)
        Instruction();

    // mot clé EXIT, fin du programme
    if (currentToken != EXIT)
        ThrowError("Mot clé 'EXIT' attendu pour terminer le programme");
    TokenSuivant(); // on sort de la partie algorithme

    // Vérification de la liste des variables déclarées pour avertir contre les variables inutilisées
    for (const auto &var : variablesDeclarees)
    {
        if (var.second.isAssigned == false) // si la variable n'a pas été assignée
            ThrowWarning("Variable déclarée mais jamais utilisée : " + var.first);
    }

    cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
}

/*
Point d'entrée de l'exécution du programme.
*/
void Program()
{
    PrintDebug("Program()");

    // premier token lu
    TokenSuivant();
    // si stdin est vide, on affiche un message d'erreur
    if (currentToken == FEOF)
        ThrowError("Aucun code à compiler, stdin vide");

    // cette partie est obligatoirement appelée car on l'utilise pour les format pour PRINT
    PartieDeclarationVariables();

    PartieAlgorithme();

    cout << "\tret\t\t# Return from main function" << endl;
    if (currentToken != FEOF)
    {
        ThrowWarning("Fin de programme déclaré, mais code restant non compilé");
    }
}

/*
Point d'entrée du compilateur.
*/
int main()
{
    Program();
}
