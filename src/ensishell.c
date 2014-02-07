/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/*
 * Calcule la taille d'un tableau de chaines.
 */
static size_t sizeOfArray(char** array) {
	if (array == NULL) {
		return 0;
	}

	size_t count = 0;
	while (array[count] != NULL) {
		count++;
	}

	return count;
}

int main() {
	printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

	while (1) {
		struct cmdline *l;
		int i, j;
		char *prompt = "ensishell>";

		l = readcmd(prompt);

		// Si l'input est fermé, on clos le programme
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			// Erreur de syntaxe, commande suivante
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) {
			printf("in: %s\n", l->in);
		}

		if (l->out) {
			printf("out: %s\n", l->out);
		}

		if (l->bg) {
			printf("background (&)\n");
		}

		// Affiche chaque commande
		for (i = 0; l->seq[i] != 0; i++) {
			char** cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j = 0; cmd[j] != 0; j++) {
				printf("'%s' ", cmd[j]);
			}
			printf("\n");
		}

		printf("\n");

		int res = fork();

		if (res == -1) {
			perror("Fork failed: ");
			exit(-1);
		} else if (res == 0) {
			char* argc = l->seq[0][0];
			int nbArgs = sizeOfArray(l->seq[0]) - 1;
			char* argv[nbArgs];
			for (i = 0; i < nbArgs; i++) {
				argv[i] = l->seq[0][i + 1];
			}
			execvp(argc, argv);
		} else {
			waitpid((pid_t)res, NULL, 0);
		}

		printf("\n");
	}
}

