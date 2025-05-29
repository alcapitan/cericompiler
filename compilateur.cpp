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
    cerr << "\tCaractères lus : " << lexer->YYText() << " (type " << current << ")" << endl;
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

void Identifier(void)
{
    PrintDebug("Identifier()");
    // si variable non assignée
    if (variablesDeclares.find(lexer->YYText()) == variablesDeclares.end())
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
    else if (variablesDeclares[lexer->YYText()] == false)
        ThrowError("Variable non assignée : " + string(lexer->YYText()));
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
    if (current == RELOP)
    {
        oprel = CompareOperator();
        ArithmeticExpression();
        cout << "\tpop	%rbx" << endl; // get first operand
        cout << "\tpop	%rax" << endl; // get second operand
        cout << "\tcmp	%rbx, %rax" << endl;
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
        cout << "\tpush	$-1" << endl; // false
        cout << "\tjmp	.endfalsecmp" << jmpId << endl;
        cout << ".truecmp" << jmpId << ":" << endl;
        cout << "\tpush	$0" << endl; // true
        cout << ".endfalsecmp" << jmpId << ":" << endl;
        jmpId++;
    }
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
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
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
    if (variablesDeclares.find(lexer->YYText()) == variablesDeclares.end())
        ThrowError("Variable non déclarée : " + string(lexer->YYText()));
    else if (variablesDeclares[nomVariableBoucle] == true)
        ThrowError("Variable boucle FOR déjà assigné hors de la boucle, risque de perte de données : " + string(lexer->YYText()));
    else
        variablesDeclares[nomVariableBoucle] = true; // on déclare la variable comme utilisée

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
        cout << "\tja .endfor" << loopJmpId << endl;
    else
        cout << "\tjb .endfor" << loopJmpId << endl;

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
    current = (TOKEN)lexer->yylex(); // on passe au token suivant

    Expression(); // on lit l'expression de la condition
    if (current != KEYWORD || strcmp(lexer->YYText(), "THEN") != 0)
        ThrowError("'THEN' attendu");
    current = (TOKEN)lexer->yylex(); // on passe au token suivant

    int blocJmpId = jmpId; // on sauvegarde l'ID du bloc pour pouvoir le réutiliser même quand il sera incrémenté par ses propres instructions
    jmpId++;

    cout << "\tpop	%rax" << endl;                 // on récupère le résultat de l'expression
    cout << "\tcmp	$0, %rax" << endl;             // on compare avec 0
    cout << "\tje	.elseif" << blocJmpId << endl; // si égal à 0, on saute à la partie ELSE

    while (strcmp(lexer->YYText(), "ENDIF") != 0 && strcmp(lexer->YYText(), "ELSE") != 0)
        Instruction(); // on exécute l'instruction dans le THEN

    cout << "\tjmp	.endif" << blocJmpId << endl; // on saute à la fin de l'IF

    cout << ".elseif" << blocJmpId << ":" << endl; // jump au bloc ELSE
    if (current == KEYWORD && strcmp(lexer->YYText(), "ELSE") == 0)
    {
        current = (TOKEN)lexer->yylex(); // on passe au token suivant
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
        else
            ThrowError("Mot-clé inconnu : " + string(lexer->YYText()));
    }
    else if (current == ID)
    {
        AssignationInstruction();

        if (current != SEMICOLON)
            ThrowError("Fin instruction inattendue, ';' manquant");
    }
    else
        ThrowError("Instruction inconnue : " + string(lexer->YYText()));

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

    if (current == DOT) // si on est déjà à la fin du programme
        ThrowWarning("Programme vide, '.' inattendu");
    else
        Instruction();
    while (current != DOT) // fin d'une instruction, on boucle si y en a plusieurs
    {
        Instruction();
    }
    if (current != DOT) // fin du programme
        ThrowError("'.' attendu");
    current = (TOKEN)lexer->yylex(); // on sort de la partie algorithme

    // Variables déclarées mais jamais utilisées
    for (const auto &var : variablesDeclares)
    {
        if (var.second == false) // si la variable n'a pas été initialisée
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
        ThrowWarning("Fin de programme déclaré, mais code restant non compilé");
    }
}
