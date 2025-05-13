#include <string>
#include <iostream>
#include <cstdlib>
using namespace std;

// ArithmeticExpression := Term {AdditiveOperator Term}
// Term := Digit | "(" ArithmeticExpression ")"
// AdditiveOperator := "+" | "-"
// Digit := "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"

char currentChar; // le caractère en train d'être lu

/*
Lit le prochain caractère non blanc (espace, tab, saut de ligne) depuis cin (entrée standard).
Met à jour currentChar.
*/
void getNextChar()
{
    while (cin.get(currentChar) && (currentChar == ' ' || currentChar == '\t' || currentChar == '\n'))
        cin.get(currentChar);
}

/*
Affiche un message d’erreur sur la sortie d’erreur standard.
Termine le programme immédiatement.
*/
void ThrowError(string s)
{
    cerr << s << endl;
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

/*
Vérifie si currentChar est un chiffre entre '0' et '9'.
Si oui, génère le code assembleur push $<digit> (empile le chiffre).
Puis lit le caractère suivant.
*/
void Digit()
{
    if ((currentChar < '0') || (currentChar > '9'))
        ThrowError("Chiffre attendu");
    else
    {
        cout << "\tpush $" << currentChar << endl;
        getNextChar();
    }
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
        Digit();
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
    char adop;
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

    // Let's proceed to the analysis and code production
    getNextChar();
    ArithmeticExpression();
    getNextChar();
    // Trailer for the gcc assembler / linker
    cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
    cout << "\tret\t\t\t# Return from main function" << endl;
    if (cin.get(currentChar))
    {
        cerr << "Caractères en trop à la fin du programme : [" << currentChar << "]";
        ThrowError("."); // unexpected characters at the end of program
    }
}
