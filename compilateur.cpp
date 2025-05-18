#include <string>
#include <iostream>
#include <cstdlib>
using namespace std;

// ArithmeticExpression := Term {AdditiveOperator Term}
// Term := Digit | "(" ArithmeticExpression ")"
// AdditiveOperator := "+" | "-"
// Digit := "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"

char currentChar; // le caractère en train d'être lu
char nextChar;    // le caractère suivant
int jmpId = 0;    // permet d'identifier chaque condition
int debug = 0;    // pour le débogage

/*
Affiche un message d’erreur sur la sortie d’erreur standard.
Termine le programme immédiatement.
*/
void ThrowError(string message)
{
    cerr << "\033[31m"; // set color to red
    cerr << "ERREUR : " << message << endl;
    // indiquer la position de l'erreur après tabulation
    cerr << "\tCaractère lu : " << currentChar << endl;
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

/*
Lit le prochain caractère non blanc (espace, tab, saut de ligne) depuis cin (entrée standard).
Met à jour currentChar et nextChar.
*/
void getNextChar()
{
    currentChar = nextChar;
    nextChar = cin.get();
    while (nextChar == ' ' || nextChar == '\t' || nextChar == '\n')
    {
        nextChar = cin.get();
    }

    PrintDebug("Caractère lu : " + string(1, currentChar));
    debug++;
    if (debug > 40)
    {
        ThrowError("Trop de caractères lus");
    }
}

// lire un nombre (suite de Digit())
void Number()
{
    PrintDebug("Number()");
    int number = 0;

    if ((currentChar < '0') || (currentChar > '9'))
        ThrowError("Chiffre attendu");

    while (currentChar >= '0' && currentChar <= '9')
    {
        number *= 10;                // Décalage à gauche
        number += currentChar - '0'; // cast char to int
        getNextChar();               // on lit le caractère suivant, soit le prochain chiffre du nombre, soit l'opérateur du chiffre
    }
    cout << "\tpush	$" << number << endl;
}

// fonction pour l'instant abstraite, définie plus bas
void ArithmeticExpression();

/*
Représente un terme de l'expression :
Soit un chiffre (via Digit()),
Soit une sous-expression entre parenthèses.
*/
void Factor()
{
    PrintDebug("Factor()");
    if (currentChar == '(')
    {
        getNextChar();
        ArithmeticExpression();
        if (currentChar != ')')
            ThrowError("')' était attendu"); // ")" expected
        else
            getNextChar();
    }
    else if (currentChar >= '0' && currentChar <= '9')
        Number(); // on détecte que ce qu'on est en train de lire est un nombre
    else
        ThrowError("'(' ou chiffre attendu");
}

void MultiplicativeOperator()
{
    PrintDebug("MultiplicativeOperator()");
    if (currentChar == '*' || currentChar == '/')
        getNextChar();
    else
        ThrowError("Opérateur multiplicatif attendu");
}

void Term()
{
    PrintDebug("Term()");
    Factor();
    // si le calcul se poursuit, ou se termine
    while (currentChar == '*' || currentChar == '/')
    {
        char operateur = currentChar;
        getNextChar();
        Factor();
        PrintDebug("calcul");
        cout << "\tpop	%rbx" << endl; // premier opérande
        cout << "\tpop	%rax" << endl; // second opérande
        if (operateur == '*')
            cout << "\timul	%rbx, %rax" << endl; // multiply both operands
        else if (operateur == '/')
            cout << "\tidiv	%rbx, %rax" << endl; // divide both operands
        cout << "\tpush	%rax" << endl;           // store result
    }
}

/*
Vérifie que currentChar est un opérateur + ou -.
Si oui, lit le caractère suivant.
Sinon, déclenche une erreur.
*/
void AdditiveOperator()
{
    PrintDebug("AdditiveOperator()");
    if (currentChar == '+' || currentChar == '-')
        getNextChar();
    else
        ThrowError("Opérateur additif attendu");
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
    char adop; // ADditive OPerator
    Term();
    // si le calcul se poursuit, ou se termine
    while (currentChar == '+' || currentChar == '-')
    {
        adop = currentChar; // Save operator in local variable
        AdditiveOperator();
        Term();
        PrintDebug("calcul");
        cout << "\tpop	%rbx" << endl; // get first operand
        cout << "\tpop	%rax" << endl; // get second operand
        if (adop == '+')
        {
            cout << "\tadd	%rbx, %rax" << endl; // add both operands
        }
        else if (adop == '-')
        {
            cout << "\tsub	%rbx, %rax" << endl; // substract both operands
        }
        cout << "\tpush	%rax" << endl; // store result
    }
}

enum CmpOperator
{
    LT, // <
    LE, // <=
    GT, // >
    GE, // >=
    EQ, // ==
    NE  // !=
};

// Expression := ArithmeticExpression [CompareOperator ArithmeticExpression]
// CompareOperator := "<" | "<=" | ">" | ">=" | "==" | "!="
CmpOperator CompareOperator()
{
    PrintDebug("CompareOperator()");
    CmpOperator opRead;
    if (nextChar == '=')
    {
        switch (currentChar)
        {
        case '<':
            opRead = LE;
            break;
        case '>':
            opRead = GE;
            break;
        case '=':
            opRead = EQ;
            break;
        case '!':
            opRead = NE;
            break;
        default:
            ThrowError("Opérateur de comparaison attendu");
            break;
        }
        getNextChar();
        getNextChar();
    }
    else
    {
        switch (currentChar)
        {
        case '<':
            opRead = LT;
            break;
        case '>':
            opRead = GT;
            break;
        default:
            ThrowError("Opérateur de comparaison attendu");
            break;
        }
        getNextChar();
    }
    return opRead;
}

void Expression()
{
    PrintDebug("Expression()");
    ArithmeticExpression();
    if (currentChar == '<' || currentChar == '>' || currentChar == '=')
    {
        CmpOperator cmpOp;
        cmpOp = CompareOperator();
        ArithmeticExpression();
        PrintDebug("comparaison");
        cout << "\tpop	%rbx" << endl; // get first operand
        cout << "\tpop	%rax" << endl; // get second operand
        cout << "\tcmp	%rbx, %rax" << endl;
        switch (cmpOp)
        {
        case LT:
            cout << "\tjl	.true" << jmpId << endl;
            break;
        case LE:
            cout << "\tjle	.true" << jmpId << endl;
            break;
        case GT:
            cout << "\tjg	.true" << jmpId << endl;
            break;
        case GE:
            cout << "\tjge	.true" << jmpId << endl;
            break;
        case EQ:
            cout << "\tje	.true" << jmpId << endl;
            break;
        case NE:
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
    cout << "# Ce code a été généré par le please-compilateur" << endl;
    cout << ".text\t\t# The following lines contain the program" << endl;
    cout << "\t.globl	main\t# The main function must be visible from outside" << endl;
    cout << "main:" << endl;
    cout << "\tmovq	%rsp, %rbp\t\t# Save the position of the stack's top" << endl;
    PrintDebug("Main()");
    getNextChar(); // initialise la lecture (la première fois c'est vide)

    // Let's proceed to the analysis and code production
    getNextChar(); // lit le premier caractère (permet de déterminer ce qu'il faut faire)
    Expression();
    getNextChar();

    // Trailer for the gcc assembler / linker
    cout << "\tmovq	%rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
    cout << "\tret\t\t\t# Return from main function" << endl;
    if (cin.get(currentChar))
    {
        ThrowError("Caractères supplémentaires à la fin de l'expression");
    }
}
