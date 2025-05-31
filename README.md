# CERIcompiler

**Auteur :** BOYER Alexandre

Projet de Licence 2 Informatique (année 2024-25) à Avignon Université.  
Compilateur écrit en C++ traduisant un langage fictif semblable à Pascal vers le langage Assembleur (64bit AT&T).

_Projet source : [Framagit Jourlin](https://framagit.org/jourlin/cericompiler)_

> ### Notes pour le correcteur :
>
> J'ai fait les TP 1 à 6 inclus.

## Documentation sur le langage

J'ai parfois divergé des consignes sur le langage reconnu, non pas par difficulté, mais par choix de conception.  
Bien que semblable à Pascal, veuillez retrouver la documentation complète dans `doc.md` et dans le fichier `test.p` qui contient des exemples de code.

## Commandes d'utilisation

Dépendances nécessaires : `g++`, `gcc`, `make`, `flex`.

Pour compiler le compilateur :

```bash
make compilateur
```

Pour compiler le programme `test.p` :

```bash
make test
./test # pour exécuter le programme compilé
```

Pour voir le code assembleur généré sur le terminal (avec les logs, les warnings et les erreurs) :

```bash
./compilateur < test.p
```
