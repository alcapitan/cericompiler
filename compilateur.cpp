#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unordered_map>
#include <FlexLexer.h>
#include "tokeniser.h"
using namespace std;

int jmpId = 0;                                 // permet d'identifier chaque condition
unordered_map<string, bool> variablesDeclares; // tableau de variables déclarées (nom, et si elles sont initialisées)

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

enum OPADD
{
    ADD,
    SUB,
    OR,
    WTFA
};

enum OPMUL
{
    MUL,
    DIV,
    MOD,
    AND,
    WTFM
};

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
    cerr << "\tCaractères lus : " << lexer->YYText() << "'(" << current << ")'" << endl;
    cerr << "\033[0m"; // reset color
    exit(-1);
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

void Identifier(void)
{
    PrintDebug("Identifier()");
    cout << "\tpush " << lexer->YYText() << endl;
    current = (TOKEN)lexer->yylex();
}

// lire un nombre (suite de Digit())
void Number()
{
    PrintDebug("Number()");
    cout << "\tpush $" << atoi(lexer->YYText()) << endl;
    current = (TOKEN)lexer->yylex();
}

// fonction pour l'instant abstraite, définie plus bas
void Expression();

/*
Représente un terme de l'expression :
Soit un chiffre (via Digit()),
Soit une sous-expression entre parenthèses.
*/
void Factor()
{
    PrintDebug("Factor()");
    if (current == RPARENT)
    {
        current = (TOKEN)lexer->yylex();
        Expression();
        if (current != LPARENT)
            ThrowError("')' était attendu");
        else
            current = (TOKEN)lexer->yylex();
    }
    else if (current == NUMBER)
        Number();
    else if (current == ID)
        Identifier();
    else
        ThrowError("'(' ou nombre ou variable attendue");
    PrintDebug("Fin de Factor()");
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

void Term()
{
    PrintDebug("Term()");
    OPMUL mulop;
    Factor();
    // si le calcul se poursuit, ou se termine
    while (current == MULOP)
    {
        mulop = MultiplicativeOperator();
        Factor();
        PrintDebug("calcul");
        cout << "\tpop	%rbx" << endl; // premier opérande
        cout << "\tpop	%rax" << endl; // second opérande

        switch (mulop)
        {
        case AND:
            cout << "\tandq	%rbx, %rax" << endl;
            cout << "\tpush %rax" << endl;
            break;
        case MUL:
            cout << "\timul %rbx, %rax" << endl;
            cout << "\tpush %rax" << endl;
            break;
        case DIV:
            cout << "\tmovq $0, %rdx" << endl; // clear remainder register
            cout << "\tidiv %rbx, %rax" << endl;
            cout << "\tpush %rax" << endl; // push result
            break;
        case MOD:
            cout << "\tmovq $0, %rdx" << endl; // clear remainder register
            cout << "\tidiv %rbx, %rax" << endl;
            cout << "\tpush %rdx" << endl; // push remainder
            break;
        default:
            ThrowError("opérateur multiplicatif attendu");
            break;
        }
    }
    PrintDebug("Fin de Term()");
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
void ArithmeticExpression()
{
    PrintDebug("ArithmeticExpression()");
    OPADD adop; // ADditive OPerator
    Term();
    // si le calcul se poursuit, ou se termine
    while (current == ADDOP)
    {
        adop = AdditiveOperator(); // Save operator in local variable
        Term();
        PrintDebug("calcul");
        cout << "\tpop	%rbx" << endl; // get first operand
        cout << "\tpop	%rax" << endl; // get second operand
        switch (adop)
        {
        case OR:
            cout << "\taddq	%rbx, %rax" << endl;
            break;
        case ADD:
            cout << "\taddq	%rbx, %rax" << endl;
            break;
        case SUB:
            cout << "\tsubq	%rbx, %rax" << endl;
            break;
        default:
            ThrowError("opérateur additif inconnu");
            break;
        }
        cout << "\tpush	%rax" << endl; // store result
    }
    PrintDebug("Fin de ArithmeticExpression()");
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

void Expression()
{
    PrintDebug("Expression()");
    OPREL oprel;
    ArithmeticExpression();
    PrintDebug("token actuel : " + string(lexer->YYText()));
    if (current == RELOP)
    {
        oprel = CompareOperator();
        ArithmeticExpression();
        PrintDebug("comparaison");
        cout << "\tpop	%rbx" << endl; // get first operand
        cout << "\tpop	%rax" << endl; // get second operand
        cout << "\tcmp	%rbx, %rax" << endl;
        switch (oprel)
        {
        case INF:
            cout << "\tjl	.true" << jmpId << endl;
            break;
        case INFE:
            cout << "\tjle	.true" << jmpId << endl;
            break;
        case SUP:
            cout << "\tjg	.true" << jmpId << endl;
            break;
        case SUPE:
            cout << "\tjge	.true" << jmpId << endl;
            break;
        case EQU:
            cout << "\tje	.true" << jmpId << endl;
            break;
        case DIFF:
            cout << "\tjne	.true" << jmpId << endl;
            break;
        default:
            ThrowError("Opérateur de comparaison attendu");
            break;
        }
        cout << "\tpush	$-1" << endl; // false
        cout << "\tjmp	.next" << jmpId << endl;
        cout << ".true" << jmpId << ":" << endl;
        cout << "\tpush	$0" << endl; // true
        cout << ".next" << jmpId << ":" << endl;
        jmpId++;
    }
    PrintDebug("Fin de Expression()");
}

void AssignationInstruction()
{
    PrintDebug("AssignationInstruction()");

    string nomVariable;
    if (current != ID)
        ThrowError("Nom de variable attendu");
    nomVariable = lexer->YYText();
    // si le nom de la variable n'est pas déclarée
    if (variablesDeclares.find(lexer->YYText()) == variablesDeclares.end())
        ThrowError("Variable non déclarée : " + current);
    else
        variablesDeclares[nomVariable] = true; // on déclare la variable comme initialisée/assignée/utilisée

    current = (TOKEN)lexer->yylex(); // on passe au token suivant
    if (current != ASSIGN)
        ThrowError("Opérateur ':=' attendu");
    else
        current = (TOKEN)lexer->yylex();

    Expression();
    //? rajout, à voir si fonctionnel
    cout << "\tpop " << nomVariable << endl;
}

void Instruction()
{
    PrintDebug("Instruction()");
    if (current == SEMICOLON)
        ThrowError("Instruction vide");
    AssignationInstruction();
    if (current != SEMICOLON)
        ThrowError("Fin instruction inattendue, ';' manquant");
    else
        current = (TOKEN)lexer->yylex(); // on sort de l'instruction
}

void PartieDeclarationVariables()
{
    PrintDebug("PartieDeclarationVariables()");
    if (current != RBRACKET)
        ThrowError("'[' attendu");
    cout << ".data" << endl;
    current = (TOKEN)lexer->yylex();

    while (current != LBRACKET)
    {
        // PrintDebug("Nom de variable : " + string(lexer->YYText()));
        if (current != ID)
            ThrowError("Nom de variable attendu");
        //  si le nom de la variable est déjà déclaré
        if (variablesDeclares.find(lexer->YYText()) != variablesDeclares.end())
        {
            ThrowError("Variable déjà déclarée : " + current);
        }
        else
        {
            variablesDeclares[lexer->YYText()] = false;                    // on déclare la variable dans notre liste
            cout << "\t" << lexer->YYText() << ":\t" << ".quad 0" << endl; // on déclare la variable
        }
        current = (TOKEN)lexer->yylex(); // on passe au token suivant
        if (current == COMMA)            // variable suivante
            current = (TOKEN)lexer->yylex();
        else if (current != LBRACKET) // si ce n'est ni , ni ] alors c'est une erreur
            ThrowError("Déclaration variables : ',' attendu");
    }
    current = (TOKEN)lexer->yylex(); // on sort de la partie déclaration variables
}

void PartieAlgorithme()
{
    cout << ".text\t\t# The following lines contain the program" << endl;
    cout << "\t.globl	main\t\t# The main function must be visible from outside" << endl;
    cout << "main:" << endl;
    cout << "\tmovq	%rsp, %rbp\t\t# Save the position of the stack's top" << endl;

    PrintDebug("Token actuel : " + string(lexer->YYText()));
    Instruction();
    while (current != DOT) // fin d'une instruction, on boucle si y en a plusieurs
    {
        Instruction();
    }
    if (current != DOT) // fin du programme
        ThrowError("'.' attendu");
    current = (TOKEN)lexer->yylex(); // on sort de la partie algorithme

    // Trailer for the gcc assembler / linker
    cout << "\tmovq	%rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
}

void Program()
{
    PrintDebug("Program()");
    if (current == RBRACKET)
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
        ThrowError("Caractères supplémentaires à la fin de l'expression");
    }
}
