# CERIcompiler

**Auteur :** BOYER Alexandre

Projet de Licence 2 Informatique à Avignon Université.  
Compilateur écrit en C++ traduisant un langage fictif semblable à Pascal vers le langage Assembleur (64bit 80x86 AT&T).

## Utilisation

Compiler le compilateur :

```bash
g++ compilateur.cpp -o compilateur
```

Utiliser le compilateur :

```bash
cat test.pls | ./compilateur > test.s
```

_Ici le fichier d'entrée contenant un code de notre langage fictif est `test.pls` et le fichier de sortie contenant le code assembleur est `test.s`._

Pour voir le code assembleur généré :

```bash
cat test.s
```

Pour compiler et exécuter le code assembleur généré :

```bash
gcc test.s -o test
./test
```
