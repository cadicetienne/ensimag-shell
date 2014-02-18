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
#include <string.h>

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

void printStringArray(char** stringArray, char* separator) {
	if (stringArray[0] == NULL) {
		printf("Empty array");
	} else {
		printf("%s", stringArray[0]);
		int i = 1;
		while (stringArray[i] != NULL) {
			printf("%s%s", separator, stringArray[i]);
			i++;
		}
	}
}

char** copyStringArray(char** stringArray) {
	int nb = 0;
	while (stringArray[nb] != NULL) {
		nb++;
	}
	char** res = malloc((nb + 1) * sizeof(char*));
	int i = 0;
	while (stringArray[i] != NULL) {
		res[i] = malloc(sizeof(char) * strlen(stringArray[i]));
		strcpy(res[i], stringArray[i]);
		i++;
	}
	res[i] = NULL;
	return res;
}

/*
 * Ajoute une cellule en tête de la liste.
 */
void add(ProcessCell** list, int pid, char** argv) {
	ProcessCell* newCell = malloc(sizeof(ProcessCell));
	newCell->pid = pid;
	newCell->argv = argv;
	newCell->next = NULL;
	if (*list == NULL) {
		*list = newCell;
	} else {
		newCell->next = *list;
		*list = newCell;
	}
}

/*
 * Imprime la liste en supprimant les processus morts.
 */
void print(ProcessCell** list) {
	if (*list == NULL) {
		printf("Aucun job en cours\n");
		return;
	}
	ProcessCell* cell = *list;
	printf("  PID  | Status | Command\n");
	printf("-------+--------+------------------------------------------\n");
	do {
		int res = waitpid(cell->pid, NULL, WNOHANG);
		printf("  %d | ", cell->pid);
		if (res == 0) {
			printf("  OK   | ");
		} else {
			printf(" DEAD  | ");
			// TODO Supprimer!
		}
		printStringArray(cell->argv, " ");
		printf("\n-------+--------+------------------------------------------\n");
		cell = cell->next;
	} while (cell != NULL);
}

// ########## //
// ## Main ## //
// ########## //

int main() {
	printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

	ProcessCell** jobs = malloc(sizeof(ProcessCell*));

	while (1) {
		struct cmdline *l;
		int i;
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
			printStringArray(cmd, " ");
			printf("\n");
		}

		printf("\n");

		///////////////
		// FIN DEBUG //
		///////////////

		// Jobs command
		if (strcmp(l->seq[0][0], "jobs") == 0) {
			print(jobs);
			continue;
		}

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
			add(jobs, res, copyStringArray(l->seq[0]));
		}
		printf("\n");
	}
}

