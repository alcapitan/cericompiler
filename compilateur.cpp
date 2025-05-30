#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <FlexLexer.h>
#include "tokeniser.h"
using namespace std;

int jmpId = 0; // permet d'identifier chaque condition
enum VariableType
{
    INT,  // entier 64 bits
    BOOL, // booléen
};

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

struct VariableProperties
{
    VariableType type; // type de la variable
    bool isAssigned;   // si la variable a une valeur assignée
    bool isConst;      // si la variable est constante
};

unordered_map<string, VariableProperties> variablesDeclarees; // liste des variables déclarées (nom, type, si elles sont assignées, si elles sont constantes)

enum OPREL
{
    EQU,
    DIFF,
    INF,
    SUP,
    INFE,
    SUPE,
    WTFR
};

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

enum OPADD
{
    ADD,
    SUB,
    OR,
    WTFA
};

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

enum OPMUL
{
    MUL,
    DIV,
    MOD,
    AND,
    WTFM
};

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

TOKEN current; // Current token

FlexLexer *lexer = new yyFlexLexer; // This is the flex tokeniser

/*
Affiche un message d’erreur sur la sortie d’erreur standard.
Termine le programme immédiatement.
*/
void ThrowError(string message)
{
    cerr << "\033[31m"; // set color to red
    cerr << "ERREUR : " << message << endl;
    cerr << "\tPosition : ligne " << lexer->lineno() << endl;
    cerr << "\tCaractères lu : " << lexer->YYText() << " (type " << current << ")" << endl;
    cerr << "\033[0m"; // reset color
    exit(-1);
}

/*
Affiche un message d’avertissement sur la sortie d’erreur standard.
*/
void ThrowWarning(string message)
{
    // set color to orange
    cerr << "\033[33m"; // set color to yellow
    cerr << "ATTENTION : " << message << endl;
    cerr << "\tPosition : ligne " << lexer->lineno() << endl;
    cerr << "\tCaractères lus : " << lexer->YYText() << " (type " << current << ")" << endl;
    cerr << "\033[0m"; // reset color
}

/*
Affiche un message de débogage sur la sortie d’erreur standard.
*/
void PrintDebug(string message)
{
    cerr << "\033[34m"; // set color to blue
    cerr << "\t# " << message << endl;
    cerr << "\033[0m"; // reset color
}

VariableType Identifier()
{
    PrintDebug("Identifier()");
    VariableType typeIdentifier = variablesDeclarees[lexer->YYText()].type;
    if (variablesDeclarees.find(lexer->YYText()) == variablesDeclarees.end())
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
    else if (variablesDeclarees[lexer->YYText()].isAssigned == false)
        ThrowError("Variable non assignée : " + string(lexer->YYText()));
    cout << "\tpush " << lexer->YYText() << endl;
    current = (TOKEN)lexer->yylex();

    return typeIdentifier;
}

// lire un nombre (suite de Digit())
VariableType Number()
{
    PrintDebug("Number()");
    // il ne peut gérer que des entiers 32 bits mais je n'arrive pas à faire une condition anti overflow
    cout << "\tpush $" << atoi(lexer->YYText()) << endl;
    current = (TOKEN)lexer->yylex();

    return INT;
}

// fonction pour l'instant abstraite, définie plus bas
VariableType Expression();

/*
Représente un terme de l'expression :
Soit un chiffre (via Digit()),
Soit une sous-expression entre parenthèses.
*/
VariableType Factor()
{
    PrintDebug("Factor()");
    VariableType typeFactor;
    if (current == RPARENT)
    {
        current = (TOKEN)lexer->yylex();
        typeFactor = Expression();
        if (current != LPARENT)
            ThrowError("')' était attendu");
        else
            current = (TOKEN)lexer->yylex();
    }
    else if (current == NUMBER)
        typeFactor = Number();
    else if (current == ID)
        typeFactor = Identifier();
    else
        ThrowError("'(' ou nombre ou variable attendue");

    return typeFactor;
}

OPMUL MultiplicativeOperator(void)
{
    PrintDebug("MultiplicativeOperator()");
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
    current = (TOKEN)lexer->yylex();
    return opmul;
}

VariableType Term()
{
    PrintDebug("Term()");
    OPMUL mulop;
    VariableType typeFactor1, typeFactor2;
    typeFactor1 = Factor();
    // si le calcul se poursuit, ou se termine
    while (current == MULOP)
    {
        mulop = MultiplicativeOperator();
        typeFactor2 = Factor();

        // vérification des types
        if (typeFactor1 != typeFactor2)
            ThrowError("Impossible de calculer, les opérandes n'ont pas le même type");

        cout << "\tpop	%rbx" << endl; // premier opérande
        cout << "\tpop	%rax" << endl; // second opérande

        switch (mulop)
        {
        case AND:
            if (typeFactor1 == INT)
                ThrowError("Opération illogique entre nombres " + to_string(mulop));
            cout << "\tandq	%rbx, %rax" << endl;
            cout << "\tpush %rax" << endl;
            break;
        case MUL:
            if (typeFactor1 == BOOL)
                ThrowError("Opération illogique entre booléens" + to_string(mulop));
            cout << "\timul %rbx, %rax" << endl;
            cout << "\tpush %rax" << endl;
            break;
        case DIV:
            if (typeFactor1 == BOOL)
                ThrowError("Opération illogique entre booléens" + to_string(mulop));
            cout << "\tmovq $0, %rdx" << endl; // clear remainder register
            cout << "\tidiv %rbx, %rax" << endl;
            cout << "\tpush %rax" << endl; // push result
            break;
        case MOD:
            if (typeFactor1 == BOOL)
                ThrowError("Opération illogique entre booléens" + to_string(mulop));
            cout << "\tmovq $0, %rdx" << endl; // clear remainder register
            cout << "\tidiv %rbx, %rax" << endl;
            cout << "\tpush %rdx" << endl; // push remainder
            break;
        default:
            ThrowError("opérateur multiplicatif attendu");
            break;
        }
    }

    return typeFactor1;
}

// AdditiveOperator := "+" | "-" | "||"
OPADD AdditiveOperator()
{
    PrintDebug("AdditiveOperator()");
    OPADD opadd;
    if (strcmp(lexer->YYText(), "+") == 0)
        opadd = ADD;
    else if (strcmp(lexer->YYText(), "-") == 0)
        opadd = SUB;
    else if (strcmp(lexer->YYText(), "||") == 0)
        opadd = OR;
    else
        opadd = WTFA;
    current = (TOKEN)lexer->yylex();
    return opadd;
}

/*
Pour chaque + ou - rencontré :
- Sauvegarde l'opérateur.
- Lit le terme suivant.
- Génère du code assembleur :
  - Récupère les deux opérandes de la pile (pop %rbx puis pop %rax)
  - Applique l'opération : addq ou subq
  - Empile le résultat final avec push %rax.
*/
VariableType ArithmeticExpression()
{
    PrintDebug("ArithmeticExpression()");
    OPADD adop; // ADditive OPerator
    VariableType typeTerm1, typeTerm2;
    typeTerm1 = Term();
    // si le calcul se poursuit, ou se termine
    while (current == ADDOP)
    {
        adop = AdditiveOperator(); // Save operator in local variable
        typeTerm2 = Term();

        // vérification des types
        if (typeTerm1 != typeTerm2)
            ThrowError("Impossible de calculer, les opérandes n'ont pas le même type");

        cout << "\tpop	%rbx" << endl; // get first operand
        cout << "\tpop	%rax" << endl; // get second operand
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
            ThrowError("opérateur additif inconnu");
            break;
        }
        cout << "\tpush	%rax" << endl; // store result
    }

    return typeTerm1;
}

OPREL CompareOperator()
{
    PrintDebug("CompareOperator()");
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
    current = (TOKEN)lexer->yylex();
    return oprel;
}

VariableType Expression()
{
    PrintDebug("Expression()");
    OPREL oprel;
    VariableType typeExpr1, typeExpr2;
    typeExpr1 = ArithmeticExpression();
    if (current == RELOP)
    {
        oprel = CompareOperator();
        typeExpr2 = ArithmeticExpression();

        // vérification des types
        if (typeExpr1 != typeExpr2)
            ThrowError("Impossible de comparer, les opérandes n'ont pas le même type");

        cout << "\tpop	%rax" << endl; // get second operand
        cout << "\tpop	%rbx" << endl; // get first operand
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
            ThrowError("Opérateur de comparaison attendu");
            break;
        }
        cout << "\tpush $0\t\t# False" << endl; // false
        cout << "\tjmp	.endcmp" << jmpId << endl;
        cout << ".truecmp" << jmpId << ":" << endl;
        cout << "\tpush $-1\t\t# True" << endl; // true
        cout << ".endcmp" << jmpId << ":" << endl;
        jmpId++;
    }

    return typeExpr1;
}

void AssignationInstruction()
{
    PrintDebug("AssignationInstruction()");

    string nomVariable;
    if (current != ID)
        ThrowError("Nom de variable attendu");
    nomVariable = lexer->YYText();
    // si le nom de la variable n'est pas déclarée
    if (variablesDeclarees.find(lexer->YYText()) == variablesDeclarees.end())
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
    else if (variablesDeclarees[nomVariable].isConst == true && variablesDeclarees[nomVariable].isAssigned == true)
        ThrowError("Variable constante déjà assigné : " + string(lexer->YYText()));
    else
        variablesDeclarees[nomVariable].isAssigned = true; // on marque la variable comme assignée

    current = (TOKEN)lexer->yylex();
    if (current != ASSIGN)
        ThrowError("Opérateur ':=' attendu");
    else
        current = (TOKEN)lexer->yylex();
    VariableType typeExpr = Expression();

    // vérification des types
    if (variablesDeclarees[nomVariable].type != typeExpr)
        ThrowError("Type de variable incompatible avec l'expression : " + nomVariable + " (variable de type " + to_string(variablesDeclarees[nomVariable].type) + ", expression de type " + to_string(typeExpr) + ")");

    cout << "\tpop " << nomVariable << endl;
}

void PrintInstruction()
{
    PrintDebug("PrintInstruction()");
    current = (TOKEN)lexer->yylex();
    VariableType typeExpr;
    typeExpr = Expression(); // on lit l'expression à afficher
    switch (typeExpr)
    {
    case INT:
        cout << "\tpop %rsi\t# The value to be displayed" << endl;
        cout << "\tmovq $FormatPrintInt, %rdi" << endl;
        cout << "\tmovl	$0, %eax" << endl;
        cout << "\tcall	printf@PLT" << endl;
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
        break;
    default:
        ThrowError("Type d'expression non géré pour l'instruction PRINT : " + string(lexer->YYText()));
    }

    if (current != SEMICOLON)
        ThrowError("Fin d'instruction ';' attendue");
}

void Instruction(); // forward declaration

void ForStatement()
{
    PrintDebug("ForStatement()");
    if (current != KEYWORD || strcmp(lexer->YYText(), "FOR") != 0)
        ThrowError("'FOR' attendu");
    current = (TOKEN)lexer->yylex();

    if (current != ID)
        ThrowError("Nom de variable attendu");
    string nomVariableBoucle = lexer->YYText();
    if (variablesDeclarees.find(lexer->YYText()) == variablesDeclarees.end())
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
    else if (variablesDeclarees[nomVariableBoucle].isAssigned == true)
        ThrowError("Variable boucle FOR déjà assigné hors de la boucle, risque de perte de données : " + string(lexer->YYText()));
    variablesDeclarees[nomVariableBoucle].isAssigned = true; // on déclare la variable comme utilisée

    current = (TOKEN)lexer->yylex();
    if (current != ASSIGN)
        ThrowError("Opérateur ':=' attendu");

    current = (TOKEN)lexer->yylex();
    ArithmeticExpression(); // valeur de départ de la variable de boucle
    cout << "\tpop " << nomVariableBoucle << endl;
    cout << "\tmovq " << nomVariableBoucle << ", %rax" << endl;

    // TO or DOWNTO
    bool loopIncrement = true; // TO = true, DOWNTO = false
    if (current == KEYWORD)
    {
        if (strcmp(lexer->YYText(), "TO") == 0)
            loopIncrement = true; // boucle croissante
        else if (strcmp(lexer->YYText(), "DOWNTO") == 0)
            loopIncrement = false; // boucle décroissante
        else
            ThrowError("'TO' ou 'DOWNTO' attendu");
    }
    else
        ThrowError("Keyword TO ou DOWNTO attendu");
    current = (TOKEN)lexer->yylex();

    ArithmeticExpression(); // valeur de fin de boucle
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

    if (current != KEYWORD || strcmp(lexer->YYText(), "DO") != 0)
        ThrowError("'DO' attendu");
    current = (TOKEN)lexer->yylex();

    while (strcmp(lexer->YYText(), "ENDFOR") != 0)
        Instruction(); // instruction dans la boucle

    cout << "\tpop " << nomVariableBoucle << endl; // on dépile la variable de boucle pour l'utiliser dans l'instruction d'incrément/decrément
    if (loopIncrement)
        cout << "\taddq $1, " << nomVariableBoucle << endl; // on incrémente la variable de boucle
    else
        cout << "\tsubq $1, " << nomVariableBoucle << endl;     // on décrémente la variable de boucle
    cout << "\tmovq " << nomVariableBoucle << ", %rax" << endl; // on met à jour la variable de boucle dans le registre
    cout << "\tjmp .for" << loopJmpId << endl;                  // on revient au début de la boucle pour revérifier la condition
    cout << ".endfor" << loopJmpId << ":" << endl;              // fin de la boucle
    if (current != KEYWORD || strcmp(lexer->YYText(), "ENDFOR") != 0)
        ThrowError("'ENDFOR' attendu");
}

void WhileStatement()
{
    PrintDebug("WhileStatement()");
    if (current != KEYWORD || strcmp(lexer->YYText(), "WHILE") != 0)
        ThrowError("'WHILE' attendu");
    current = (TOKEN)lexer->yylex();
    int loopJmpId = jmpId; // on sauvegarde l'ID de la boucle pour pouvoir le réutiliser même quand il sera incrémenté par ses propres instructions
    jmpId++;               // on l'incrémente maintenant pour la différencier de ses propres instructions

    cout << ".while" << loopJmpId << ":" << endl; // condition de la boucle
    Expression();                                 // expression de la condition
    cout << "\tpop	%rax" << endl;
    cout << "\tcmp	$0, %rax" << endl;
    cout << "\tje	.endwhile" << loopJmpId << endl; // si la condition est fausse, on sort de la boucle
    if (current != KEYWORD || strcmp(lexer->YYText(), "DO") != 0)
        ThrowError("'DO' attendu");
    current = (TOKEN)lexer->yylex(); // sinon on exécute la ligne suivante d'assembleur

    while (strcmp(lexer->YYText(), "ENDWHILE") != 0)
        Instruction(); // ici la condition est vraie

    cout << "\tjmp	.while" << loopJmpId << endl;    // on revient au début de la boucle pour revérifier la condition
    cout << ".endwhile" << loopJmpId << ":" << endl; // on sort de la boucle
    if (current != KEYWORD || strcmp(lexer->YYText(), "ENDWHILE") != 0)
        ThrowError("'ENDWHILE' attendu");
}

void IfStatement()
{
    PrintDebug("IfStatement()");
    if (current != KEYWORD || strcmp(lexer->YYText(), "IF") != 0)
        ThrowError("'IF' attendu");
    current = (TOKEN)lexer->yylex();

    Expression(); // on lit l'expression de la condition
    if (current != KEYWORD || strcmp(lexer->YYText(), "THEN") != 0)
        ThrowError("'THEN' attendu");
    current = (TOKEN)lexer->yylex();

    int blocJmpId = jmpId; // on sauvegarde l'ID du bloc pour pouvoir le réutiliser même quand il sera incrémenté par ses propres instructions
    jmpId++;

    cout << "\tpop	%rax" << endl;               // on récupère le résultat de l'expression
    cout << "\tcmp	$0, %rax" << endl;           // on compare avec 0
    cout << "\tje	.else" << blocJmpId << endl; // si égal à 0, on saute à la partie ELSE

    while (strcmp(lexer->YYText(), "ENDIF") != 0 && strcmp(lexer->YYText(), "ELSE") != 0)
        Instruction(); // on exécute l'instruction dans le THEN

    cout << "\tjmp	.endif" << blocJmpId << endl; // on saute à la fin de l'IF

    cout << ".else" << blocJmpId << ":" << endl; // jump au bloc ELSE
    if (current == KEYWORD && strcmp(lexer->YYText(), "ELSE") == 0)
    {
        current = (TOKEN)lexer->yylex();
        while (strcmp(lexer->YYText(), "ENDIF") != 0)
            Instruction(); // on exécute l'instruction dans le ELSE
    }

    if (current != KEYWORD || strcmp(lexer->YYText(), "ENDIF") != 0)
        ThrowError("'ENDIF' attendu");
    else
        cout << ".endif" << blocJmpId << ":" << endl; // on sort de l'IF
}

void Instruction()
{
    PrintDebug("Instruction()");
    if (current == SEMICOLON)
        ThrowWarning("Instruction vide");
    else if (current == KEYWORD)
    {
        if (strcmp(lexer->YYText(), "IF") == 0)
            IfStatement();
        else if (strcmp(lexer->YYText(), "WHILE") == 0)
            WhileStatement();
        else if (strcmp(lexer->YYText(), "FOR") == 0)
            ForStatement();
        else if (strcmp(lexer->YYText(), "PRINT") == 0)
            PrintInstruction();
        else
            ThrowError("Mot-clé inattendu : " + string(lexer->YYText()));
    }
    else if (current == ID)
    {
        AssignationInstruction();

        if (current != SEMICOLON)
            ThrowError("Fin instruction inattendue, ';' manquant");
    }
    else if (current == VARDECL)
        ThrowError("Les variables doivent être déclarées avant toute instruction");
    else
        ThrowError("Instruction inconnue : " + string(lexer->YYText()));

    current = (TOKEN)lexer->yylex(); // on sort de l'instruction
}

void PartieDeclarationVariables()
{
    PrintDebug("PartieDeclarationVariables()");
    cout << ".data" << endl;
    cout << "\t.align 8" << endl;
    cout << "\tFormatPrintInt: .string \"%lld\\n\"\t# print 64-bit signed integers" << endl;
    cout << "\tFormatPrintTrue: .string \"True\"\t# print boolean true" << endl;
    cout << "\tFormatPrintFalse: .string \"False\"\t# print boolean false" << endl;
    if (current != VARDECL)
        return; // il n'y a pas de variables à déclarer

    // on boucle tant que le token est DECLARE de VARDECL
    while (current == VARDECL)
    {
        // Instruction DECLARE obligatoire
        if (strcmp(lexer->YYText(), "DECLARE") != 0)
            ThrowError("Une déclaration de variable doit commencer par 'DECLARE'");

        // vérification si déclaration de variable constante
        current = (TOKEN)lexer->yylex();
        bool isConst = false; // si la variable est constante
        if (current == VARDECL && strcmp(lexer->YYText(), "CONST") == 0)
        {
            isConst = true; // si la variable est constante
            current = (TOKEN)lexer->yylex();
            PrintDebug("Variable constante");
        }
        else
            PrintDebug("Variable non constante");

        // Type de variable
        if (current != VARTYPE)
            ThrowError("Type de variable manquant après 'DECLARE'");
        VariableType typeVariable;
        if (strcmp(lexer->YYText(), "INT") == 0)
            typeVariable = INT;
        else if (strcmp(lexer->YYText(), "BOOL") == 0)
            typeVariable = BOOL;
        else
            ThrowError("Type de variable inconnu : " + string(lexer->YYText()));
        PrintDebug("Type de variable : " + string(lexer->YYText()));

        // Nom de variable
        current = (TOKEN)lexer->yylex();
        if (current != ID)
            ThrowError("Nom de variable manquant après le type de variable");
        string nomVariable = lexer->YYText();

        current = (TOKEN)lexer->yylex();
        // si assignation dès la déclaration
        if (current == ASSIGN)
        {
            current = (TOKEN)lexer->yylex();
            if (typeVariable == INT)
            {
                if (current != NUMBER)
                    ThrowError("Les expressions ne sont pas autorisées dans la déclaration de variable");
                cout << "\t" << nomVariable << ":\t" << ".quad " << lexer->YYText() << endl;
            }
            else if (typeVariable == BOOL)
            {
                if (current != BOOLVAL)
                    ThrowError("Valeur booléenne attendue pour la variable constante");
                if (strcmp(lexer->YYText(), "True") == 0)
                    cout << "\t" << nomVariable << ":\t" << ".quad -1\t\t# True" << endl;
                else if (strcmp(lexer->YYText(), "False") == 0)
                    cout << "\t" << nomVariable << ":\t" << ".quad 0\t\t# False" << endl;
                else
                    ThrowError("Valeur booléenne inconnue : " + string(lexer->YYText()));
            }
            else
                ThrowError("Type de variable inconnu : " + string(lexer->YYText()));

            variablesDeclarees[nomVariable] = {typeVariable, true, isConst}; // type de la variable, si elle est assignée, si elle est constante

            current = (TOKEN)lexer->yylex();
        }
        else
            variablesDeclarees[nomVariable] = {typeVariable, false, isConst}; // type de la variable, si elle est assignée, si elle est constante

        // point-virgule
        if (current != SEMICOLON)
            ThrowError("Fin de déclaration de variable manquante, ';' attendu");
        if (current == VARDECL)
        {
            PrintDebug("Prochaine déclaration de variable");
        }
        current = (TOKEN)lexer->yylex(); // on sort de la partie déclaration variables
    }
}

void PartieAlgorithme()
{
    PrintDebug("PartieAlgorithme()");
    cout << ".text\t\t# The following lines contain the program" << endl;
    cout << "\t.globl	main\t\t# The main function must be visible from outside" << endl;
    cout << "main:" << endl;
    cout << "\tmovq	%rsp, %rbp\t\t# Save the position of the stack's top" << endl;

    if (current == EXIT) // si on est déjà à la fin du programme
        ThrowWarning("Aucune instruction trouvée, programme vide");
    else
        Instruction();
    while (current != EXIT) // fin d'une instruction, on boucle si y en a plusieurs
    {
        Instruction();
    }
    if (current != EXIT) // fin du programme
        ThrowError("'.' attendu");
    current = (TOKEN)lexer->yylex(); // on sort de la partie algorithme

    // Variables déclarées mais jamais utilisées
    for (const auto &var : variablesDeclarees)
    {
        if (var.second.isAssigned == false) // si la variable n'a pas été initialisée
        {
            ThrowWarning("Variable déclarée mais jamais utilisée : " + var.first);
        }
    }

    // Trailer for the gcc assembler / linker
    cout << "\tmovq	%rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
}

void Program()
{
    PrintDebug("Program()");

    // cette partie est obligatoirement appelée car on l'utilise pour les format pour PRINT
    PartieDeclarationVariables();

    PartieAlgorithme();
}

/*
- Écrit l’en-tête assembleur attendu par le compilateur GNU.
- Lit une expression arithmétique depuis l'entrée standard.
- Gère les erreurs si des caractères supplémentaires sont présents à la fin.
- Écrit la fin du programme assembleur.
*/
int main()
{
    // First version : Source code on standard input and assembly code on standard output
    // Header for gcc assembler / linker
    PrintDebug("Main()");

    current = (TOKEN)lexer->yylex();
    Program();

    cout << "\tret\t\t\t# Return from main function" << endl;
    if (current != FEOF)
    {
        ThrowWarning("Fin de programme déclaré, mais code restant non compilé");
    }
}
