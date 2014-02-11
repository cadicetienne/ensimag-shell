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

// ########### //
// ## Liste ## //
// ########### //

typedef struct process_cell {
	int pid;
	char** argv;
	struct process_cell* next;
} ProcessCell;

/*
 * Ajoute une cellule en tête de la liste.
 */
void add(ProcessCell** list, int pid, char** argv) {
	// TODO
}

/*
 * Imprime la liste en supprimant les processus morts.
 */
void print(ProcessCell** list) {
	// TODO
}

// ########## //
// ## Main ## //
// ########## //

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

		///////////
		// DEBUG //
		///////////

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

		///////////////
		// FIN DEBUG //
		///////////////

		// Fork
		int res = fork();
		if (res == -1) {
			perror("fork failed: ");
			exit(-1);
		} else if (res == 0) { // Dans le fils
			// Appel d'execvp
			execvp(l->seq[0][0], l->seq[0]);

			// Code appelé uniquement si execvp retourne
			// == si execvp échoue
			perror("execvp failed: ");
			exit(-1);
		}

		// Dans le père
		if (!l->bg) {
			waitpid((pid_t)res, NULL, 0);
		} else {
			
		}
		printf("\n");
	}
}

