// Nom           : shell_lib.c
// Rôle          : Bibliothèque pour le Shell personnalisé
// Auteur        : Ronan LOSSOUARN
// Date          : 14/05/2022
// Licence       : L2 SYSTÈME D'EXPLOITATION

// Compilation   : gcc -Wall -fpic -shared shell_lib.c -o shell_lib.so

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
    MaxPathLength = 512, // longueur max d'un nom de fichier
};

// Découpe une chaîne de caractères et la place dans un vecteur de chaînes de caractères
// Arguments : la chaîne a découper, le caractère de séparation,
// le vecteur cible, et le nombre max de mot
// Retourne : le nombre d'élément de la ligne de commande
int decouper(char * ligne, char * separ, char * mot[], int maxmot){
    int i;
    mot[0] = strtok(ligne, separ);
    
    for (i = 1 ; mot[i -1] != 0 ; ++i){
        if (i == maxmot){
        fprintf(stderr, "Erreur dans la fonction decouper : trop de mots\n") ;
            mot[i-1] = 0;
            break ;
        }

        mot[i]=strtok(NULL,separ);
    }

    return i;
}

// Change l'emplacement du répertoire courant
// Arguments : nombre d'élements sur la ligne de commandes, la ligne de commande
int moncd(int n_ldc, char * ldc[]){
    char * dir;
    int t;
    
    if (n_ldc < 2){ // si aucun argument
        dir = getenv("HOME");
        if (dir == 0)
            dir = "/tmp";
        }

    else if (n_ldc > 2){ // si trop d'arguments
        fprintf(stderr, "usage : %s [dir]\n", ldc[0]);
        return 1;
    }

    else //sinon séletionne le chemin indiqué en argument
        dir = ldc[1];
    
    t = chdir(dir);

    if (t < 0){
        perror(dir);
        return 1;
    }

    return 0;
}

// Retourne la page de documentation d'une des trois commandes interne
// Arguments : la ligne de commande, nombre d'élements sur la ligne de commandes,
// la liste des commandes internes, la commande désiré et le chemin vers la racine du programme
void man2(char * ldc[], int n_ldc, char * liste_commandes[], char arg[], char path[]){
    // valeur arbitraire de switch_args pour l'accès à default dans switch
    int switch_args = 3, i; 
    
    char pathname[MaxPathLength];

    // compare la commande exécutée avec les commandes internes
    for (i = 0; i < 3; i++){
    // si c'est une commande interne, sélectionne le bon switch
        if (strcmp(arg, liste_commandes[i]) == 0){
            switch_args = i; 
            break;
        }
    }

    // remplace man2 par man en premier argument
    // avec comme argument le chemin vers le fichier correspondant
    switch (switch_args){
        case 0:
            ldc[0] = "man";
            snprintf(pathname, sizeof pathname, "%s/%s.gz", path, arg);
            ldc[1] = strdup(pathname);
        case 1:
            ldc[0] = "man";
            snprintf(pathname, sizeof pathname, "%s/%s.gz", path, arg);
            ldc[1] = strdup(pathname);
        case 2:
            ldc[0] = "man";
            snprintf(pathname, sizeof pathname, "%s/%s.gz", path, arg);
            ldc[1] = strdup(pathname);
        default:
            if (n_ldc > 2)
                perror("argument man2");
            break;
    }
}

// Sélections des commandes internes du Shell
// Arguments : nombre d'élements sur la ligne de commandes, la ligne de commande
int commandes_internes(int n_ldc, char * ldc[], char path[]){
    // valeur arbitraire de switch_args pour l'accès à default dans switch
    int n_commandes = 3, switch_args = 3, i; 
    
    char * liste_commandes[n_commandes];
    // liste des commandes internes
    liste_commandes[0] = "exit";
    liste_commandes[1] = "moncd";
    liste_commandes[2] = "man2";//  en construction

    // compare la commande exécutée avec les commandes internes
    for (i = 0; i < n_commandes; i++){
        // si c'est une commande interne, sélectionne le bon switch
        if (strcmp(ldc[0], liste_commandes[i]) == 0){
            switch_args = i; 
            break;
        }
    }

    // switch pour sélectionner la bonne commande
    switch (switch_args){
        case 0:
            exit(0);
        case 1:
            moncd(n_ldc, ldc);
            return 1;
        case 2:
            if (n_ldc != 2){
                perror("arguments man2");
                return 1;
            }
            man2(ldc, n_ldc, liste_commandes, ldc[1], path);
        default:
            break;
    }

    return 0;
}

// Analyse la ligne de commande pour rediriger les entrées et les sorties
// Arguments : la ligne de commande, sa longueur et un vecteur vide
// pour accueillir place la ldc sans les arguments de redirections
// Retourne : un vecteurs de 2 drapeaux, [0] = quel exec utiliser, [1] nmb de pipes
int * analyse_ldc(char * ldc[], int n_ldc, char * ldc_analysees[][MaxMot]){
    char * buffer_file;
    int in, out, cleanidx = 0, i = 0;
    static int flags[2];
    // vecteurs de 3 drapeaux : 
    // [0] = quel exec utiliser, [1] nmb de pipe, [2] arrière-plan
    flags[0] = 0, flags[1] = 0, flags[2] = 0;
    
    for (int j = 0; j < n_ldc-1; j++){ // redirection de la sortie standard
        if (!strcmp(ldc[j], ">")){
            ++j;
            flags[0] = 1;
            buffer_file = ldc[j], // mémorise le fichier pour 2>&1
            out = creat(ldc[j], 0644); // créer un nouveau fichier
            dup2(out, STDOUT_FILENO); // redirige la sortie standard vers le fichier
            close(out); // ferme le flux
            continue;
        }
    
        // redirection de la sortie standard avec ajout
        if (!strcmp(ldc[j], ">>")){
            ++j;
            flags[0] = 2;
            buffer_file = ldc[j], // mémorise le fichier pour 2>&1
            out = open(ldc[j], O_CREAT | O_RDWR | O_APPEND, 0644);
            dup2(out, STDOUT_FILENO);
            close(out);
            continue;
        }

        // redirection de la sortie d'erreur
        if (!strcmp(ldc[j], "2>")){
            ++j;
            flags[0] = 3;
            buffer_file = ldc[j],
            out = creat(ldc[j], 0644);
            dup2(out, STDERR_FILENO);
            close(out);
            continue;
        }
        
        // redirection de la sortie d'erreur avec ajout
        if (!strcmp(ldc[j], "2>>")){
            ++j;
            flags[0] = 4;
            buffer_file = ldc[j], // mémorise le fichier pour 2>&1
            out = open(ldc[j], O_CREAT | O_RDWR | O_APPEND, 0644);
            dup2(out, STDERR_FILENO);
            close(out);
            continue;
        }
        
        // redirection de la sortie d'erreur comme la sortie standard
        if (!strcmp(ldc[j], "2>&1")){
            // switch pour selectionner le type de redirection (> ou >>)
            switch (flags[0]){
                case 1:
                    out = creat(buffer_file, 0644);
                    break;
                case 2:
                    out = open(buffer_file, O_CREAT | O_RDWR | O_APPEND, 0644);
                    break;
                default:
                    fprintf(stderr, "Aucune redirection indiquée précédemment.\n") ;
                    break;
            }
            dup2(out, STDERR_FILENO);
            close(out);
            continue;
        }
        
        // redirection de la sortie standard comme la sortie d'erreur
        if (!strcmp(ldc[j], "1>&2")){
            switch (flags[0]){
                case 3:
                    out = creat(buffer_file, 0644); // create new output file
                    break;
                case 4:
                    out = open(buffer_file, O_CREAT | O_RDWR | O_APPEND, 0644);
                    break;
                default:
                    fprintf(stderr, "Aucune redirection indiquée précédemment.\n") ;
                    break;
            }
            dup2(out, STDOUT_FILENO);
            close(out);
            continue;
        }

        // redirection de l'entrée standard
        if (!strcmp(ldc[j], "<")){
            ++j;
            flags[0] = 5;
            if ((in = open(ldc[j], O_RDONLY)) < 0) // ouvre le fichier en lecture
                fprintf(stderr, "error opening file\n");
            dup2(in, STDIN_FILENO);
            close(in);
            continue;
        }
        
        // compte le nombre de pipe
        if (!strcmp(ldc[j], "|")){
            flags[0] = 6;
            ++flags[1] ; // ajoute 1 au nombre de pipe
            ++i; // accès au prochain élément du tableau ldc_analysees
            // remise à zéro de l'index pour le prochain élément de ldc_analysees
            cleanidx = 0;
            continue;
        }

        // arrière plan
        if (!strcmp(ldc[j], "&")){
            flags[2] = 1;
            continue;
        }

        // création de/des ligne(s) de commande sans les arguments de redirections
        ldc_analysees[i][cleanidx++] = ldc[j];
        ldc_analysees[i][cleanidx] = NULL;
    }

    return flags;
}

// Lance une commande avec ou sans argument via le fonction execv
// Réinitialise toutes les sorties précédemment redirigées.
// Arguments : Les répetoires du PATH, la ligne de commande,
// la copie des sorties originales
void exec_redirections(char * dirs[], char * ldc[][MaxMot], int STD[], int arr_plan){
    char pathname[MaxPathLength];
    int tmp = fork(); // lance processus enfant
    if (tmp < 0){
        perror("fork");
        return;
    }

    if (tmp != 0){   // parent : attendre la fin de l'enfant
        if (arr_plan == 0) // exécution en premier-plan
            while(wait(0) != tmp) break ;
        // réinitialisation des redirections
        dup2(STD[0], STDIN_FILENO);
        dup2(STD[1], STDOUT_FILENO);
        dup2(STD[2], STDERR_FILENO);
        return;
    }

    // enfant : exec du programme
    for (int i = 0; dirs[i] != 0; i++){
        snprintf(pathname, sizeof pathname, "%s/%s", dirs[i], ldc[0][0]);
        execv(pathname, ldc[0]);
    }
    
    // aucun exec n'a fonctionne
    fprintf(stderr, "%s:not found\n", ldc[0][0]);
    exit(1);
}

// Lance des commandes et redirige les sorties liées aux pipes
// Arguments : Les répetoires du PATH, la ligne de commande,
// la copie des sorties originales, le nombre de pipes
void exec_pipes(char *dirs[], char *ldc[][MaxMot], int STD[], int flags[]){
    char pathname[MaxPathLength];
    int k, i = 0, tmp[3];
    int n_pipes = flags[1];
    int arr_plan = flags[2];
    int pipefds[2 * n_pipes]; // descripteurs de fichier des pipes

    for (i = 0; i < n_pipes; i++){ // boucle qui fabrique les pipes
        if (pipe(pipefds + i * 2) < 0){
            perror("Fabrication de pipe");
            return;
        }
    }

    int j = 0;
    for (k = 0; k < n_pipes + 1; ++k){
        tmp[k] = fork();
        // enfant :
        if (tmp[k] == 0){
            if (k != 0){ // si ce n'est pas la première commande
                // redirige l'entrée dans le pipe
                if (dup2(pipefds[j - 2], STDIN_FILENO) < 0){
                    perror("dup2, entrée de pipe"); /// j-2 0 j+1 1
                    return;
                }
            }

            if (k != n_pipes){ // si ce n'est pas la dernière commande
                // redirige la sortie dans le pipe
                if (dup2(pipefds[j + 1], STDOUT_FILENO) < 0){
                    perror("dup2, sortie de pipe");
                    return;
                }
            }

            for (i = 0; i < 2 * n_pipes; i++) // ferme tous les pipes
                close(pipefds[i]);

            for (i = 0; dirs[i] != 0; i++){ // lance la commande
                snprintf(pathname, sizeof pathname, "%s/%s", dirs[i], ldc[k][0]);
                execv(pathname, ldc[k]);
            }
        }

        else if (tmp[k] < 0){
            fprintf(stderr, "%s:not found\n", ldc[0][0]);
            exit(1);
        }

        j += 2; // sélection des descripteurs de fichier du prochain pipe
    }
    // parent : 
    for (i = 0; i < 2 * n_pipes; i++) // ferme tous les pipes
        close(pipefds[i]);

    for (i = 0; i < k; i++){ // attendre la fin des enfants
        if (arr_plan == 0) // exécution en premier-plan
            while (wait(0) != tmp[i]) break;
    }

    // réinitialisation des redirections
    dup2(STD[0], STDIN_FILENO);
    dup2(STD[1], STDOUT_FILENO);
    dup2(STD[2], STDERR_FILENO);

    return;
}

