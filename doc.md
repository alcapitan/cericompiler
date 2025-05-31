# Documentation sur mon propre langage compilé

Veuillez retrouver un code d'exemple complet dans le fichier `test.p`.

## Les commentaires

Les commentaires sont ignorés par le compilateur et prennent cette forme :

```
/* Ceci est un commentaire */
/* Un autre
commentaire */
```

## Les instructions

L'instruction `EXIT` (remplaçant le `.`) permet d'arrêter le programme.  
Les instructions sont écrites en majuscules et terminées obligatoirement par un point-virgule (à l'exception de `EXIT`).  
Le saut à la ligne n'est pas pris en compte par le compilateur.

## Déclaration de variables

Les types supportés sont : `INT` (entier signé) et `BOOL` (booléen où `-1` signifie `True` et `0` signifie `False`).  
La déclaration doit se faire avant toute instruction.

```
DECLARE INT a;
DECLARE BOOL b;
DECLARE CONST INT c;
DECLARE CONST BOOL d := True;
```

Le compilateur empêche :

-   L'utilisation de variables non initialisées.
-   L'utilisation de variables non déclarées.
-   La réassignation de constantes.

## La manipulation de variables

Les opérateurs pour les entiers sont : `+`, `-`, `*`, `/`, `%` (modulo).  
Les opérateurs pour les booléens sont : `&&` (and), `||` (or), `<`, `<=`, `>`, `>=`, `==`, `!=`.

```
a := 5;
b := True;
c := (a + 3) * 2;
```

Le compilateur empêche :

-   L'assignation d'une valeur de type incompatible.
-   L'utilisation d'opérateurs pour un type incompatible.

## L'affichage

L'instruction d'affichage est `PRINT` et attend une expression (de type `INT` ou `BOOL`).

```
a := 3;
PRINT a + 2;
PRINT b || (a > c);
```

## Les conditions et les boucles

La condition `IF` finis par `ENDIF`, elle prend aussi `ELSE` facultativement, mais pas de `ELSE IF`.

```
IF c > 0 THEN
    a := a + a;
    b := True;
ELSE
    a := a * a;
    b := False;
ENDIF
```

La boucle `WHILE` finis par `ENDWHILE`.

```
WHILE a < 10 DO
    PRINT a;
    a := a + 1;
ENDWHILE
```

La boucle `FOR` est de la forme :

```
/* attention la boucle FOR compte de 0 à 10 inclus */
FOR i := 0 TO 10 DO
    PRINT i;
ENDFOR
/* alternativement */
FOR i := 10 DOWNTO 0 DO
    PRINT i;
ENDFOR
```

> À noter que j'ai un soucis de valeur de `i` écrasé lors ce qu'un calcul est fait dans la boucle, ce qui entraîne des comportements incorrects.
