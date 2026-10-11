#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

#define LARGEUR 20
#define HAUTEUR 16
#define FICHIER_SAUVEGARDE "sauvegarde.dat"
#define SCORE_MINIMUM (-50)

#define RESET   "\033[0m"
#define CYAN    "\033[36m"
#define JAUNE   "\033[33m"
#define VERT    "\033[32m"
#define ROUGE   "\033[31m"
#define GRAS    "\033[1m"
#define EFFACER "\033[H\033[J"

typedef struct {
    int position;
    int objets[HAUTEUR][LARGEUR];
    int score;
    int taille;
    int difficulte;
} Jeu;

static struct termios ancien_terminal;
static int terminal_configure = 0;

static void restaurer_terminal(void) {
    if (terminal_configure) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &ancien_terminal);
        terminal_configure = 0;
    }
}

static int config_terminal(void) {
    struct termios nouveau_terminal;
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &ancien_terminal) == -1)
        return 0;

    nouveau_terminal = ancien_terminal;
    nouveau_terminal.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    nouveau_terminal.c_cc[VMIN] = 1;
    nouveau_terminal.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &nouveau_terminal) == -1)
        return 0;

    terminal_configure = 1;
    atexit(restaurer_terminal);
    return 1;
}

static int touche_disponible(long attente_ms) {
    fd_set ensemble;
    struct timeval delai;
    FD_ZERO(&ensemble);
    FD_SET(STDIN_FILENO, &ensemble);
    delai.tv_sec = attente_ms / 1000;
    delai.tv_usec = (attente_ms % 1000) * 1000;
    return select(STDIN_FILENO + 1, &ensemble, NULL, NULL, &delai) > 0;
}

static void pause_ms(long ms) {
    struct timespec delai;
    delai.tv_sec = ms / 1000;
    delai.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&delai, NULL);
}

static Jeu init_jeu(int difficulte) {
    Jeu j;
    memset(&j, 0, sizeof j);
    j.position = LARGEUR / 2;
    j.score = 0;
    j.difficulte = difficulte;
    j.taille = (difficulte == 1) ? 2 : (difficulte == 2 ? 1 : 0);
    return j;
}

static void afficher_jeu(const Jeu *j) {
    printf(EFFACER);
    printf(CYAN GRAS "          LE SAUVETAGE EN MER\n" RESET);
    printf(" Attrape les objets qui tombent avec ton radeau !\n\n");

    printf(CYAN "  ");
    for (int i = 0; i < LARGEUR; ++i) printf("■ ");
    printf(RESET "\n");

    for (int k = 1; k < HAUTEUR - 1; ++k) {
        printf(CYAN "  ■ " RESET);
        for (int i = 1; i < LARGEUR - 1; ++i) {
            if (k == HAUTEUR - 2 && abs(i - j->position) <= j->taille) {
                printf(JAUNE GRAS "██" RESET);
            } else if (j->objets[k][i]) {
                printf(ROUGE GRAS "✹ " RESET);
            } else {
                printf("  ");
            }
        }
        printf(CYAN "■\n" RESET);
    }

    printf(CYAN "  ");
    for (int i = 0; i < LARGEUR; ++i) printf("■ ");
    printf(RESET "\n\n");

    const char *niveau = j->difficulte == 1 ? "Facile" :
                         (j->difficulte == 2 ? "Moyen" : "Difficile");
    printf(GRAS "  Score : " RESET "%s%d%s    " GRAS "Niveau : " RESET "%s%s%s\n",
           j->score < 0 ? ROUGE : VERT, j->score, RESET, JAUNE, niveau, RESET);
    printf("  [a] gauche  [d] droite  [s] sauvegarder  [c] charger  [q] quitter\n");
    printf("  Les touches fonctionnent sans appuyer sur Entree.\n");
    fflush(stdout);
}

static void deplacer(Jeu *j, char direction) {
    int min_pos = 1 + j->taille;
    int max_pos = LARGEUR - 2 - j->taille;
    if (direction == 'a' && j->position > min_pos) --j->position;
    if (direction == 'd' && j->position < max_pos) ++j->position;
}

static void mise_a_jour_objets(Jeu *j) {
    /* Traiter les objets qui arrivent sur la ligne du radeau. */
    for (int i = 1; i < LARGEUR - 1; ++i) {
        if (j->objets[HAUTEUR - 2][i]) {
            if (abs(i - j->position) <= j->taille) ++j->score;
            else --j->score;
        }
    }

    /* Descendre les objets d'une ligne. */
    for (int k = HAUTEUR - 2; k >= 2; --k)
        for (int i = 1; i < LARGEUR - 1; ++i)
            j->objets[k][i] = j->objets[k - 1][i];

    for (int i = 1; i < LARGEUR - 1; ++i)
        j->objets[1][i] = 0;

    /* Apparitions aleatoires, avec une frequence selon la difficulte. */
    int tirage = rand() % 3;
    int apparait = (j->difficulte == 1) ? (tirage == 0)
                  : (j->difficulte == 2) ? (tirage != 0)
                  : 1;

    if (apparait) {
        int colonne = 1 + rand() % (LARGEUR - 2);
        j->objets[1][colonne] = 1;
        if (j->difficulte == 3 && rand() % 2 == 0) {
            int autre = 1 + rand() % (LARGEUR - 2);
            j->objets[1][autre] = 1;
        }
    }
}

static int sauvegarder_partie(const Jeu *j) {
    FILE *f = fopen(FICHIER_SAUVEGARDE, "wb");
    if (!f) return 0;
    int ok = fwrite(j, sizeof *j, 1, f) == 1;
    if (fclose(f) != 0) ok = 0;
    return ok;
}

static int charger_partie(Jeu *j) {
    FILE *f = fopen(FICHIER_SAUVEGARDE, "rb");
    if (!f) return 0;
    Jeu tmp;
    int ok = fread(&tmp, sizeof tmp, 1, f) == 1;
    fclose(f);
    if (!ok || tmp.difficulte < 1 || tmp.difficulte > 3 ||
        tmp.position < 1 || tmp.position >= LARGEUR - 1 ||
        tmp.taille < 0 || tmp.taille > 2)
        return 0;
    *j = tmp;
    return 1;
}

static int choisir_difficulte(void) {
    int choix = 0;
    printf(CYAN GRAS "\n       LE SAUVETAGE EN MER\n\n" RESET);
    puts("Attrape les objets qui tombent et protege ton score.\n");
    puts("  1. Facile    - radeau large, rythme lent");
    puts("  2. Moyen     - difficulte equilibree");
    puts("  3. Difficile - radeau etroit, rythme rapide");
    printf("\nTon choix (1-3) : ");
    fflush(stdout);

    while (scanf("%d", &choix) != 1 || choix < 1 || choix > 3) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        printf("Choix invalide. Entre 1, 2 ou 3 : ");
        fflush(stdout);
    }
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    return choix;
}

int main(void) {
    srand((unsigned int)time(NULL));
    Jeu j = init_jeu(choisir_difficulte());

    if (!config_terminal()) {
        fprintf(stderr,
            "\nCe terminal ne permet pas la lecture des touches en direct.\n"
            "Utilise un vrai terminal Linux/WSL ou un terminal integre compatible.\n");
        return EXIT_FAILURE;
    }

    int quitter = 0;
    int message_ticks = 0;

    while (!quitter && j.score >= SCORE_MINIMUM) {
        afficher_jeu(&j);

        /* Le jeu avance meme si le joueur ne touche aucune touche.
           Une touche est lue immediatement si elle est disponible. */
        long intervalle = j.difficulte == 1 ? 420 :
                          (j.difficulte == 2 ? 300 : 210);
        if (touche_disponible(intervalle)) {
            char touche;
            if (read(STDIN_FILENO, &touche, 1) == 1) {
                if (touche == 'q') quitter = 1;
                else if (touche == 'a' || touche == 'd') deplacer(&j, touche);
                else if (touche == 's') {
                    restaurer_terminal();
                    printf("\n%s\n", sauvegarder_partie(&j) ?
                           "Partie sauvegardee." : "Echec de la sauvegarde.");
                    printf("Appuie sur Entree pour reprendre...");
                    (void)getchar();
                    (void)config_terminal();
                } else if (touche == 'c') {
                    restaurer_terminal();
                    printf("\n%s\n", charger_partie(&j) ?
                           "Partie chargee." : "Aucune sauvegarde valide.");
                    printf("Appuie sur Entree pour reprendre...");
                    (void)getchar();
                    (void)config_terminal();
                }
            }
        }

        /* Chute aleatoire independante des deplacements. */
        mise_a_jour_objets(&j);
        if (message_ticks > 0) --message_ticks;
    }

    restaurer_terminal();
    afficher_jeu(&j);
    if (j.score < SCORE_MINIMUM)
        printf(ROUGE "\nPartie terminee : le score est passe sous -50.\n" RESET);
    printf(GRAS "Score final : %d\n" RESET, j.score);
    return EXIT_SUCCESS;
}
