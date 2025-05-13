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

/*
Lit le prochain caractère non blanc (espace, tab, saut de ligne) depuis cin (entrée standard).
Met à jour currentChar et nextChar.
*/
void getNextChar()
{
    currentChar = nextChar;
    while (cin.get(nextChar) && (nextChar == ' ' || nextChar == '\t' || nextChar == '\n'))
        cin.get(nextChar);
}

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
Vérifie que currentChar est un opérateur + ou -.
Si oui, lit le caractère suivant.
Sinon, déclenche une erreur.
*/
void AdditiveOperator()
{
    if (currentChar == '+' || currentChar == '-')
        getNextChar();
    else
        ThrowError("Opérateur additif attendu");
}

// lire un nombre (suite de Digit())
void Number()
{
    int number = 0;

    if ((currentChar < '0') || (currentChar > '9'))
        ThrowError("Chiffre attendu");

    while (currentChar >= '0' && currentChar <= '9')
    {
        number *= 10;                // Décalage à gauche
        number += currentChar - '0'; // cast char to int
        getNextChar();
    }
    cout << "\tpush $" << number << endl;
}

// fonction pour l'instant abstraite, définie plus bas
void ArithmeticExpression();

/*
Représente un terme de l'expression :
Soit un chiffre (via Digit()),
Soit une sous-expression entre parenthèses.
*/
void Term()
{
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
        Number();
    else
        ThrowError("'(' ou chiffre attendu");
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
    char adop; // ADditive OPerator
    Term();
    while (currentChar == '+' || currentChar == '-')
    {
        adop = currentChar; // Save operator in local variable
        AdditiveOperator();
        Term();
        cout << "\tpop %rbx" << endl; // get first operand
        cout << "\tpop %rax" << endl; // get second operand
        if (adop == '+')
            cout << "\taddq	%rbx, %rax" << endl; // add both operands
        else
            cout << "\tsubq	%rbx, %rax" << endl; // substract both operands
        cout << "\tpush %rax" << endl;           // store result
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
    cout << "\t.globl main\t# The main function must be visible from outside" << endl;
    cout << "main:" << endl;
    cout << "\tmovq %rsp, %rbp\t\t# Save the position of the stack's top" << endl;
    getNextChar();

    // Let's proceed to the analysis and code production
    getNextChar();
    ArithmeticExpression();
    getNextChar();
    // Trailer for the gcc assembler / linker
    cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
    cout << "\tret\t\t\t# Return from main function" << endl;
    if (cin.get(currentChar))
    {
        ThrowError("Caractères supplémentaires à la fin de l'expression");
    }
}
