# Fichiers
COMPILATEUR_SOURCE = compilateur.cpp
COMPILATEUR_EXECUTABLE = compilateur
TEST_SOURCE = test.pls
TEST_ASSEMBLEUR = test.s
TEST_EXECUTABLE = test

# Toujours exécuter les cibles
.PHONY: all build test run clean

# Comportement par défaut
all: clean build run

# Compiler le compilateur
build:
	@echo "Compilation du compilateur..."
	g++ ${COMPILATEUR_SOURCE} -o ${COMPILATEUR_EXECUTABLE}

# Traduire test.pls → assembleur
test:
	@echo "Traduction du fichier test.pls en assembleur..."
	cat ${TEST_SOURCE} | ./${COMPILATEUR_EXECUTABLE} > ${TEST_ASSEMBLEUR} || (echo "Erreur de compilation" && exit 1)
	@echo "Contenu du fichier assembleur généré :"
	cat ${TEST_ASSEMBLEUR}

# Compiler et exécuter
run: test
	@echo "Compilation du fichier assembleur..."
	gcc ${TEST_ASSEMBLEUR} -o ${TEST_EXECUTABLE}
	@echo "Exécution du programme..."
	./${TEST_EXECUTABLE}

# Nettoyage
clean:
	@echo "Nettoyage des fichiers générés..."
	rm -f ${COMPILATEUR_EXECUTABLE} ${TEST_ASSEMBLEUR} ${TEST_EXECUTABLE}
