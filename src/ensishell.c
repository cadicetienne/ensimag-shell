/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>
#include <string.h>

#include "variante.h"
#include "readcmd.h"
#include "string.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/*
 * Structure de liste de processus
 */
typedef struct process_cell {
	int pid;
	char** argv;
	struct process_cell* next;
} ProcessCell;

/**
 * Imprime un tableau de chaines de caractères
 */

int getNbArgs(char ** stringArray) {
	int i = 0;
	while (stringArray[i]) {
		i++;
	}
	return i;
}

/**
 * Transforme le tableau d'arguments passés en paramètres
 * pour extraire les expressions régulières. Le tableau
 * est finalement modifié.
 */
void convertRegexp(char *** stringArray) {
	wordexp_t p;
	char **w;
	int j;
	int size = getNbArgs(*stringArray);
	char ** finalArgs;
	int totalSize = 0;
	char *** args = malloc((size + 1) * sizeof(char**));
	
	int i = 0;

	// On stocke d'abord chaque élément de résultat dans un tableau
	// intermédiaire
	while ((*stringArray)[i]) {
		wordexp((*stringArray)[i], &p, 0);
		w = p.we_wordv;
	
		args[i] = malloc((p.we_wordc + 1) * sizeof(char*));
		totalSize += p.we_wordc;
		
		for (j = 0 ; j < p.we_wordc ; j++) {
			args[i][j] = malloc(strlen(w[j]) * sizeof(char));
			strcpy(args[i][j], w[j]);
		}
		args[i][j] = NULL;
		i++;
	}
	args[size] = NULL;
	finalArgs = malloc((totalSize + 1)* sizeof(char**));
	finalArgs[totalSize] = NULL;
	i = 0;
	int curr = 0;

	//On remplit ensuite le tableau final	
	while (args[i] && curr < totalSize){
		j = 0;
		while (args[i][j] != NULL && curr < totalSize) {
			finalArgs[curr] = malloc(strlen(args[i][j]) * sizeof(char));
			strcpy(finalArgs[curr], args[i][j]);
			j++;
			curr++;
		}
		i++;
	}
	// On libère la mémoire
	char ** tmp = *stringArray;
	free(tmp);

	// On remplace la valeur pointée par le nouveau tableau
	*stringArray = finalArgs;
	free(args);
	
}

void printStringArray(char** stringArray, char* separator) {
	if (!stringArray[0]) {
		printf("Empty array");
	} else {
		printf("%s", stringArray[0]);
		int i = 1;
		while (stringArray[i]) {
			printf("%s%s", separator, stringArray[i]);
			i++;
		}
	}
}


/*
 * Copie l'ensemble des commandes comme un seul tableau de chaines
 */
char** copyCommandsAsStringArray(char*** commands) {
	int nbStrings = 0, i = 0, j;
	while (commands[i]) {
		j = 0;
		while (commands[i][j]) {
			j++;
		}
		i++;
		nbStrings += j + 1;
	}
	char** res = malloc((nbStrings) * sizeof(char*));
	int cmdIndex = 0, cmdPartIndex;
	i = 0;
	while (commands[cmdIndex]) {
		cmdPartIndex = 0;
		while (commands[cmdIndex][cmdPartIndex]) {
			res[i] = malloc(sizeof(char) * strlen(commands[cmdIndex][cmdPartIndex]));
			strcpy(res[i], commands[cmdIndex][cmdPartIndex]);
			cmdPartIndex++;
			i++;
		}
		res[i] = "|";
		cmdIndex++;
		i++;
	}
	res[--i] = NULL;
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
	if (!*list) {
		*list = newCell;
	} else {
		newCell->next = *list;
		*list = newCell;
	}
}

/*
 * Imprime la liste des jobs en supprimant les jobs morts.
 */
void print(ProcessCell** list) {
	if (!*list) {
		printf("Aucun job en cours\n");
		return;
	}
	ProcessCell* prev = NULL;
	ProcessCell* cell = *list;
	printf("  PID  | Status | Command\n");
	printf("-------+--------+------------------------------------------\n");
	do {
		int delete = 0;
		int res = waitpid(cell->pid, NULL, WNOHANG);
		printf("  %d | ", cell->pid);
		if (res == 0) {
			printf("  OK   | ");
		} else {
			delete = 1;
			printf(" DEAD  | ");
		}
		printStringArray(cell->argv, " ");
		printf("\n-------+--------+------------------------------------------\n");
		if (delete) {
			if (!prev) {
				*list = cell->next;
				free(cell->argv);
				free(cell);
				cell = *list;
			} else {
				prev->next = cell->next;
				free(cell->argv);
				free(cell);
				cell = prev->next;
			}
		} else {
			prev = cell;
			cell = cell->next;
		}
	} while (cell);
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
		char *prompt = "\nensishell>";

		l = readcmd(prompt);

		// Si l'input est fermé, on clos le programme
		if (!l) {
			printf("exit\n");
			exit(EXIT_SUCCESS);
		}

		if (l->err) {
			// Erreur de syntaxe, commande suivante
			printf("error: %s\n", l->err);
			continue;
		}

		if (!l->seq[0]) {
			// Aucune commande entrée
			continue;
		}

		///////////
		// DEBUG //
		///////////
		/*
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
		for (i = 0; l->seq[i]; i++) {
			convertRegexp(&(l->seq[i]));	
			char** cmd = l->seq[i];
			printf("seq[%d]: ", i);
			printStringArray(cmd, " ");
			printf("\n");
		}

		printf("\n");
		*/
		///////////////
		// FIN DEBUG //
		///////////////

		// Commande jobs
		if (strcmp(l->seq[0][0], "jobs") == 0) {
			print(jobs);
			continue;
		}

		int pipefd[2];
		int inFd = 0;
		int outFd = 0;
		int res;

		if (l->in) {
			inFd = open(l->in, O_RDONLY);
			if (inFd == -1) {
				printf("Failed to open file '%s' in read mode\n", l->in);
				continue;
			}
		}

		if (l->out) {
			outFd = open(l->out, O_WRONLY|O_CREAT, S_IRWXU);
			if (outFd == -1) {
				printf("Failed to open/create file '%s' in write mode\n", l->out);
				continue;
			}
		}

		for (i = 0; l->seq[i]; i++) {
			if (pipe(pipefd) == -1) {
				perror("pipe failed");
				exit(EXIT_FAILURE);
			}
			res = fork();
			if (res == -1) {
				perror("fork failed");
				exit(EXIT_FAILURE);
			} else if (res == 0) {
				// Branchement de l'entrée précédente (éventuellment
				// l'entrée standard) sur l'entrée du processus
				dup2(inFd, 0);
				if (l->seq[i + 1]) {
					// Pas la dernière commande, on branche la sortie
					// pour la récupérer dans l'entrée suivante
					dup2(pipefd[1], 1);
				} else if (outFd) {
					// Dernière commande, on branche sur la sortie out
					dup2(outFd, 1);
				}
				close(pipefd[0]);
				execvp(l->seq[i][0], l->seq[i]);
				perror("execvp failed");
				exit(EXIT_FAILURE);
			} else {
				inFd = pipefd[0];
				close(pipefd[1]);
			}
		}

		if (!l->bg) {
			waitpid(res, NULL, 0);
		} else {
			add(jobs, res, copyCommandsAsStringArray(l->seq));
		}
	}
}

