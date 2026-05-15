#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "BackSpace.h"
#include "PageRank.h"

int compare_doubles(const void *a, const void *b) {
    // qsort envoie des pointeurs génériques.
    // comme on trie des double on les convertit en pointeurs vers double.
    const double *pa = a;
    const double *pb = b;
    // On récupère les deux valeurs à comparer.
    double x = *pa;
    double y = *pb;
    // tri croissant : le plus petit doit venir avant.
    if(x < y) {
        return -1;
    }

    if(x > y) {
        return 1;
    }

    // Même valeur.
    return 0;
}

void tracer_nuage_points_gnuplot(int N, double* pi_google, double* pi_final) {
    printf("\nGeneration du nuage de points Gnuplot \n");
    //chaque point represente un noeud avec son score google et son score backspace
    FILE* data_file = fopen("scatter_data.dat", "w");
    if (!data_file) {
        printf("Erreur : Impossible de creer le fichier.\n");
        return;
    }

    //min_val commence a 1 car tous les scores sont entre 0 et 1
    //max_val commence a 0 pour etre sur que le premier score le depasse
    double min_val = 1.0, max_val = 0.0;
    for(int i = 0; i < N; i++) {
        if(pi_google[i] > 0 && pi_final[i] > 0) {
            //on evite les score a 0 car apres on passe en echelle log
            fprintf(data_file, "%d %e %e\n", i, pi_google[i], pi_final[i]);
            //on suit le min et le max pour bien regler les axes du graphe
            if(pi_google[i] > max_val) max_val = pi_google[i];
            if(pi_google[i] < min_val) min_val = pi_google[i];
            if(pi_final[i] > max_val) max_val = pi_final[i];
            if(pi_final[i] < min_val) min_val = pi_final[i];
        }
    }
    fclose(data_file);

    //on ouvre gnuplot en mode pipe, -persistent garde la fenetre ouverte apres la fin du programme
    FILE* gnuplot = popen("gnuplot -persistent", "w");
    if (gnuplot==NULL){
        return;
    }

    //on met les deux axes en log pour mieux voir les petits scores
    fprintf(gnuplot, "set title 'Google PR vs Backspace PR (Comparaison par Noeud)' font ',14'\n");
    fprintf(gnuplot, "set xlabel 'Score Google PR (Log)'\n");
    fprintf(gnuplot, "set ylabel 'Score Backspace PR (Log)'\n");

    fprintf(gnuplot, "set logscale xy\n");
    fprintf(gnuplot, "set format x '10^{%%L}'\n");
    fprintf(gnuplot, "set format y '10^{%%L}'\n");
    fprintf(gnuplot, "set grid\n");

    //on met les deux axes a la meme echelle pour que la droite y=x soit vraiment a 45 degres
    fprintf(gnuplot, "set size square\n");
    //on ajoute une marge autour des donnees pour pas que les points collent aux bords
    fprintf(gnuplot, "set xrange [%e:%e]\n", min_val/2, max_val*2);
    fprintf(gnuplot, "set yrange [%e:%e]\n", min_val/2, max_val*2);

    //la droite y=x montre les noeuds qui n'ont pas vraiment changer de score
    fprintf(gnuplot, "plot x with lines lc rgb 'black' lw 2 title 'Y = X (Aucun changement)', \\\n");
    //fprintf(gnuplot, "     'scatter_data.dat' using 2:3 with points pt 7 ps 0.5 lc rgb 'blue' title 'Noeuds'\n");
    fprintf(gnuplot, "     'scatter_data.dat' using 2:3 with points pt 7 ps 0.2 lc rgb 'blue' title 'Noeuds'\n");
    pclose(gnuplot);
}

void liberer_graphe(int** adj, int N){
    //on verifie que adj est pas deja null sinon free plante
    if(adj != NULL){
        //liberer chaque ligne puis le tableau principal
        for(int i = 0; i < N; i++){
            free(adj[i]);
        }
        free(adj);
    }
}

void verifier_vecteur_proba(double* pi, int N, const char* nom){
    //permet de verifier que le PageRank reste bien normaliser
    double somme = somme_vecteur(pi, N);
    printf("\nSomme %s = %.12f\n", nom, somme);

    //EPSILON est defini dans PageRank.h, on accepte une petite erreur numerique
    if(fabs(somme - 1.0) < EPSILON){
        printf("Verification OK : %s est un vecteur de probabilite\n", nom);
    }else{
        printf("Erreur : %s ne somme pas a 1\n", nom);
    }
}

void afficher_graphe(int N, int** adj, int* deg_sortant, const char* titre){
    printf("\n%s\n", titre);
    //affichage simple des voisins sortants utile surtout pour les petits test
    for(int i = 0; i < N; i++){
        printf("%d -> ", i);

        for(int k = 0; k < deg_sortant[i]; k++){
            printf("%d ", adj[i][k]);
        }

        printf("\n");
    }
}

void fusionner_pagerank(
    int N,
    double* pi_backspace,
    int* nouveau_id,
    int* premiere_copie,
    NoeudPuit* impasses,
    int nb_impasses,
    double* pi_final
){
    //le pagerank backspace est calcule sur le graphe agrandi,
    // ici on le remet sur les anciens sommets
    //on initialise tout a 0 avant de remplir
    for(int i = 0; i < N; i++) {
        pi_final[i] = 0.0;
    }

    //premier passage : les sommets normaux (pas des puits)
    for(int i = 0; i < N; i++) {
        if(nouveau_id[i] != -1) {
            //sommet normal : son score est juste recopie avec son nouvel id
            pi_final[i] = pi_backspace[nouveau_id[i]];
        }
        //si nouveau_id vaut -1 c'est une impasse, on la traite dans la boucle suivante
    }

    //deuxieme passage : les puits, leur score est disperse dans plusieurs copies
    for(int i = 0; i < nb_impasses; i++) {
        int id_original = impasses[i].id_original;
        //premiere_copie donne l'indice du debut des copies dans le grand graphe
        int id_copie_start = premiere_copie[id_original];
        int deg_in = impasses[i].degre_entrant;

        double somme_puits = 0.0;
        for(int j = 0; j < deg_in; j++) {
            //pour un puit on additionne toute ses copies pour retrouver son score final
            somme_puits += pi_backspace[id_copie_start + j];
        }

        pi_final[id_original] = somme_puits;
    }
}

void comparer_resultats(int N, double* pi_google, double* pi_final, NoeudPuit* impasses, int nb_impasses) {
    printf("\nComparaison des peres d'impasses (Google vs Backspace) \n");

    //tableau booleen pour marquer seulement les peres d'impasses
    //calloc initialise tout a 0 directement, pas besoin de boucle
    int* est_pere_impasse = calloc(N, sizeof(int));
    if (est_pere_impasse == NULL) {
        printf("Erreur allocation memoire pour est_pere_impasse\n");
        return;
    }

    //on parcourt chaque impasse et on marque ses voisins entrants comme peres
    int nb_peres_uniques = 0;
    for (int i = 0; i < nb_impasses; i++) {
        for (int j = 0; j < impasses[i].degre_entrant; j++) {
            int pere = impasses[i].voisins_entrants[j];
            if (est_pere_impasse[pere] == 0) {
                //on compte le pere une seul fois meme si il pointe vers plusieurs puits
                est_pere_impasse[pere] = 1;
                nb_peres_uniques++;
            }
        }
    }

    printf("Trouve %d peres uniques pointant vers des impasses.\n\n", nb_peres_uniques);
    printf("%-10s | %-18s | %-18s | %-18s\n", "Noeud", "Score Google", "Score Backspace", "Difference (B - G)");
    printf("\n");

    //on affiche seulement les sommets qui sont parents d'au moins une impasse
    for (int i = 0; i < N; i++) {
        if (est_pere_impasse[i] == 1) {
            //difference positive veut dire que Backspace donne plus de poids a ce noeud
            double diff = pi_final[i] - pi_google[i];

            printf("%-10d | %.12f   | %.12f   | %+.12f\n",
                   i, pi_google[i], pi_final[i], diff);
        }
    }

    free(est_pere_impasse);
}

int main(int argc, char* argv[]){

    if (argc < 2) {
        //si l'utilisateur donne pas de fichier on arrete directement
        printf("Usage : %s <nom_du_fichier.mtx>\n", argv[0]);
        printf("Exemple : %s test2.mtx\n", argv[0]);
        return 1;
    }

    //seed aleatoire base sur le temps pour avoir des impasses differentes a chaque execution
    srand(time(NULL));

    //variables du graphe original
    int N;
    int** adj = NULL;
    int* deg_sortant = NULL;

    //variables pour savoir qui arrive vers chaque sommet
    int** adj_entrant = NULL;
    int* deg_entrant = NULL;

    //liste des sommets puits detecter dans le graphe
    NoeudPuit* puits = NULL;
    int nb_puits = 0;

    // Lecture du graphe original
    //cette fonction remplit toutes les structures en une seule passe sur le fichier
    printf("\nLecture du fichier : %s\n", argv[1]);
    lire_et_trouver_impasses(
        argv[1],
        &N,
        &adj,
        &deg_sortant,
        &adj_entrant,
        &deg_entrant,
        &puits,
        &nb_puits
    );

    // afficher_graphe(N, adj, deg_sortant, "Graphe original");

    //  Si aucun puits, on cree quelques impasses aleatoirement
    if(nb_puits == 0){
        printf("\nAucune impasse detectee : creation aleatoire d'impasses.\n");

        //on supprime les anciennes infos entrantes car les degres sortants vont changer
        free(deg_entrant);
        free(puits);
        liberer_graphe(adj_entrant, N);

        for(int t = 0; t < 3; t++){
            //on force quelques impasses pour pouvoir tester Backspace meme sur un graphe sans puits
            creer_impasse_aleatoire(N, deg_sortant);
        }

        //on remet tout a NULL avant de recalculer sinon on ecrase des pointeurs valides
        adj_entrant = NULL;
        deg_entrant = NULL;
        puits = NULL;
        nb_puits = 0;

        recalculer_impasses(
            N,
            adj,
            deg_sortant,
            &adj_entrant,
            &deg_entrant,
            &puits,
            &nb_puits
        );
    }

    printf("\nNombre de puits utilises = %d\n", nb_puits);

    //  PageRank Google classique sur le graphe original/modifie
    double* pi_google = malloc(N * sizeof(double));

    if(pi_google == NULL){
        printf("Erreur allocation pi_google\n");
        exit(1);
    }

    printf("\nPageRank Google classique\n");
    //premier calcul de reference, avec la methode habituelle des noeud pui
    true_pagerank(N, adj, deg_sortant, pi_google);
    afficher_pagerank(N, pi_google);
    verifier_vecteur_proba(pi_google, N, "PageRank Google");

    //  Construction du graphe Backspace
    int N2;
    //N2 est la taille du nouveau graphe apres avoir remplacer les puits par des copies
    int** adj2 = NULL;
    int* deg_sortant2 = NULL;

    int* nouveau_id = NULL;
    int* premiere_copie = NULL;
    //nouveau_id et premiere_copie servent a faire le lien entre ancien et nouveau graphe

    //cette fonction construit le graphe elargi avec les copies des noeuds puits
    construire_graphe_backspace(
        N,
        adj,
        deg_sortant,
        puits,
        nb_puits,
        &adj2,
        &deg_sortant2,
        &N2,
        &nouveau_id,
        &premiere_copie
    );

    // afficher_graphe(N2, adj2, deg_sortant2, "Graphe Backspace");

    //la difference N2 - N correspond au nombre de copies creees pour les puits
    printf("\nN = %d, N2 = %d\n", N, N2);

    // PageRank Backspace
    //pi_backspace a la taille N2 et pas N car le graphe a grandi
    double* pi_backspace = malloc(N2 * sizeof(double));

    if(pi_backspace == NULL){
        printf("Erreur allocation pi_backspace\n");
        exit(1);
    }

    printf("\nPageRank Backspace \n");
    //on lance le meme PageRank mais sur le graphe transforme par Backspace
    true_pagerank(N2, adj2, deg_sortant2, pi_backspace);
    // afficher_pagerank(N2, pi_backspace);
    //normalement la somme doit rester a 1 meme si le graphe a plus de sommet
    verifier_vecteur_proba(pi_backspace, N2, "PageRank Backspace");

    //  Fusion et compare
    //on revient a la taille N pour pouvoir comparer avec google
    double* pi_final = malloc(N * sizeof(double));
    //pi_final aura de nouveau la taille du graphe original
    if(pi_final == NULL){
        printf("Erreur allocation pi_final\n");
        exit(1);
    }

    //on reconstruit le vecteur de taille N a partir du vecteur de taille N2
    fusionner_pagerank(N, pi_backspace, nouveau_id, premiere_copie, puits, nb_puits, pi_final);
    verifier_vecteur_proba(pi_final, N, "PageRank Final (Fusionne)");

    //afficher ensuite les valeurs finales sur les id du graphe de depart
    afficher_pagerank(N, pi_final);
    //le tableau de comparaison des peres d'impasses, c'est le resultat principal
    comparer_resultats(N, pi_google, pi_final, puits, nb_puits);

    //genere le fichier scatter_data.dat et ouvre gnuplot si il est installe
    tracer_nuage_points_gnuplot(N, pi_google, pi_final);

    // liberation memoire
    //tout ce qui a ete malloc/calloc doit etre liberer a la fin
    free(pi_google);
    free(pi_backspace);
    free(pi_final);
    free(nouveau_id);
    free(premiere_copie);

    //liberer_graphe s'occupe des tableaux 2D
    liberer_graphe(adj, N);
    liberer_graphe(adj_entrant, N);
    liberer_graphe(adj2, N2);

    free(deg_sortant);
    free(deg_entrant);
    free(deg_sortant2);
    free(puits);

    return 0;
}
