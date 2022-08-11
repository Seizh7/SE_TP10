// Nom           : shell.c
// Rôle          : Shell personnalisé pour le porjet final du chapitre 10
// du cours Systèmes d'Exploitation
// Auteur        : Ronan LOSSOUARN
// Date          : 14/05/2022
// Licence       : L2 SYSTÈME D'EXPLOITATION

// Compilation   : gcc -Wall shell.c shell_lib.so -o shell -lreadline

// SYNOPSIS
// ./shell

// DESCRIPTION
// Bibliothèque de fonction pour le Shell personnalisé incluant 
// comme commandes interne et fonctions :
// - une commande moncd, commande cd personnalisé
// - une commande exit, permettant d'éteindre le processus du shell
// - une commande man2 pour afficher les documentation des commandes internes de ce shell
// - la redirection des entrées, sorties et des erreurs
// - lancement de commandes reliées par des pipes
// - l'exécution des commandes en arrières-plan
// - l'auto-complétion des noms de fichiers

// Fonctions importées
#include <sys.h>
#include <readline/readline.h>
#include <readline/history.h>

// Variables globales
enum{
    MaxLigne = 2024, // longueur max d'une ligne de commandes
    MaxMot = MaxLigne / 2, // nbre max de mot dans la ligne
    MaxCommande = MaxMot / 2, // nbre max de commande
    MaxDirs = 100, // nbre max de repertoire dans PATH
};

// Prototypes
int decouper(char *, char *, char *[], int);
int moncd(int, char *[]);
int commandes_internes(int , char *[], char []);
int * analyse_ldc(char *[], int, char *[][MaxMot], char []);
void exec_simple(char *[], char *[][MaxMot], int);
void exec_redirections(char *[], char *[][MaxMot], int[], int);
void exec_pipes(char *[], char *[][MaxMot], int[], int[]);

// Définition d'expression
#define PROMPT "? "

int main(int ac, char * av[]){
    char * ligne; // ligne de commande lu par readline
    char * ldc[MaxMot]; // ligne de commande retournée par la fonction découper
    char * ldc_analysees[MaxCommande][MaxMot]; // ligne de commande après avoir été analysée
    char * dirs[MaxDirs];
    char * original_path = getenv("PWD"); // sauvegarde du chemin d'origine du programme
    int n_ldc; // nombre d'éléments dans la ligne de commande
    int * flags; 
    // copie des sorties standards pour les réinitialiser
    int STD[3] = {dup(STDIN_FILENO), dup(STDOUT_FILENO), dup(STDERR_FILENO)};

    // decoupe une copie du PATH
    decouper(strdup(getenv("PATH")), ":", dirs, MaxDirs);

    // changement de l'utilisation de TAB lors de readline pour l'auto-complétion
    rl_bind_key('\t', rl_complete);

    // lit et traite chaque ligne de commande
    while ((ligne = readline(PROMPT)) != 0) {
        // découpe la ligne et récupère le nombre d'élements présents
        n_ldc = decouper(ligne, " \t\n", ldc, MaxMot);

        if (ldc[0] == 0) // ligne vide
            continue;

        // si la commande est une commane interne
        if (commandes_internes(n_ldc - 1, ldc, original_path)) 
            continue;

        // analyse la ligne de commande, pour trouver des redirections ou des pipes
        flags = analyse_ldc(ldc, n_ldc, ldc_analysees, ldc[1]) ;

        
        // exécution du/des processus suivants le flag indiqué
        if (flags[0] <= 5)
            exec_redirections(dirs, ldc_analysees, STD, flags[2]);

        else
            exec_pipes(dirs, ldc_analysees, STD, flags);
    }

    printf("Bye\n");
    return 0;
}