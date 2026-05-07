#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "BackSpace.h"
#include "PageRank.h"

int compare_doubles(const void *a, const void *b) {
    double arg1 = *(const double*)a;
    double arg2 = *(const double*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

void calculer_stats(double* arr_trie, int N, double* std_dev, double* gini) {
    double mean = 1.0 / N;
    double variance = 0.0;
    double sum_i_xi = 0.0;

    for(int i = 0; i < N; i++) {
        double diff = arr_trie[i] - mean;
        variance += diff * diff;
        
        sum_i_xi += (i + 1) * arr_trie[i];
    }
    
    *std_dev = sqrt(variance / N);
    *gini = (2.0 / N) * sum_i_xi - (double)(N + 1) / N;
}

void tracer_courbe_gnuplot(int N, double* pi_google, double* pi_final) {
    printf("\n--- Generation du Graphique Gnuplot ---\n");

    double* sorted_google = malloc(N * sizeof(double));
    double* sorted_backspace = malloc(N * sizeof(double));
    
    for(int i = 0; i < N; i++) {
        sorted_google[i] = pi_google[i];
        sorted_backspace[i] = pi_final[i];
    }
    
    qsort(sorted_google, N, sizeof(double), compare_doubles);
    qsort(sorted_backspace, N, sizeof(double), compare_doubles);

    double std_google, gini_google;
    double std_backspace, gini_backspace;
    calculer_stats(sorted_google, N, &std_google, &gini_google);
    calculer_stats(sorted_backspace, N, &std_backspace, &gini_backspace);

    printf("Statistiques Google    : StdDev = %.8f, Gini = %.4f\n", std_google, gini_google);
    printf("Statistiques Backspace : StdDev = %.8f, Gini = %.4f\n", std_backspace, gini_backspace);

    FILE* data_file = fopen("plot_data.dat", "w");
    if (!data_file) {
        printf("Erreur : Impossible de creer le fichier de donnees.\n");
        free(sorted_google); free(sorted_backspace);
        return;
    }
    for(int i = 0; i < N; i++) {
        fprintf(data_file, "%d %e %e\n", i, sorted_google[i], sorted_backspace[i]);
    }
    fclose(data_file);

    FILE* gnuplot = popen("gnuplot -persistent", "w");
    if (!gnuplot) {
        printf("Erreur : Gnuplot introuvable. Assurez-vous qu'il est installe.\n");
        free(sorted_google); free(sorted_backspace);
        return;
    }

    fprintf(gnuplot, "set title 'Distribution des Scores PageRank (Triés)' font ',14'\n");
    fprintf(gnuplot, "set xlabel 'Noeuds (triés du plus petit au plus grand)'\n");
    fprintf(gnuplot, "set ylabel 'Score PageRank (Echelle Logarithmique)'\n");
    
    fprintf(gnuplot, "set logscale y\n");
    fprintf(gnuplot, "set format y '10^{%%L}'\n");
    fprintf(gnuplot, "set grid\n");
    fprintf(gnuplot, "set key top left\n");

    fprintf(gnuplot, "set label 1 'Google    : StdDev = %.2e, Gini = %.4f' at graph 0.05, 0.80 textcolor rgb 'blue' font ',11'\n", std_google, gini_google);
    fprintf(gnuplot, "set label 2 'Backspace : StdDev = %.2e, Gini = %.4f' at graph 0.05, 0.73 textcolor rgb 'red' font ',11'\n", std_backspace, gini_backspace);

    fprintf(gnuplot, "plot 'plot_data.dat' using 1:2 with lines lw 2 lc rgb 'blue' title 'Google PR', \\\n");
    fprintf(gnuplot, "     'plot_data.dat' using 1:3 with lines lw 2 lc rgb 'red' title 'Backspace PR'\n");

    pclose(gnuplot);
    free(sorted_google);
    free(sorted_backspace);
}

void tracer_nuage_points_gnuplot(int N, double* pi_google, double* pi_final) {
    printf("\n--- Generation du Scatter Plot Gnuplot ---\n");

    FILE* data_file = fopen("scatter_data.dat", "w");
    if (!data_file) {
        printf("Erreur : Impossible de creer le fichier.\n");
        return;
    }
    
    double min_val = 1.0, max_val = 0.0;
    for(int i = 0; i < N; i++) {
        if(pi_google[i] > 0 && pi_final[i] > 0) {
            fprintf(data_file, "%d %e %e\n", i, pi_google[i], pi_final[i]);
            if(pi_google[i] > max_val) max_val = pi_google[i];
            if(pi_google[i] < min_val) min_val = pi_google[i];
            if(pi_final[i] > max_val) max_val = pi_final[i];
            if(pi_final[i] < min_val) min_val = pi_final[i];
        }
    }
    fclose(data_file);

    FILE* gnuplot = popen("gnuplot -persistent", "w");
    if (!gnuplot) return;

    fprintf(gnuplot, "set title 'Google PR vs Backspace PR (Comparaison par Noeud)' font ',14'\n");
    fprintf(gnuplot, "set xlabel 'Score Google PR (Log)'\n");
    fprintf(gnuplot, "set ylabel 'Score Backspace PR (Log)'\n");
    
    fprintf(gnuplot, "set logscale xy\n");
    fprintf(gnuplot, "set format x '10^{%%L}'\n");
    fprintf(gnuplot, "set format y '10^{%%L}'\n");
    fprintf(gnuplot, "set grid\n");
    
    fprintf(gnuplot, "set size square\n");
    fprintf(gnuplot, "set xrange [%e:%e]\n", min_val/2, max_val*2);
    fprintf(gnuplot, "set yrange [%e:%e]\n", min_val/2, max_val*2);

    fprintf(gnuplot, "plot x with lines lc rgb 'black' lw 2 title 'Y = X (Aucun changement)', \\\n");
    fprintf(gnuplot, "     'scatter_data.dat' using 2:3 with points pt 7 ps 0.5 lc rgb 'blue' title 'Noeuds'\n");

    pclose(gnuplot);
}

void liberer_graphe(int** adj, int N){
    if(adj != NULL){
        for(int i = 0; i < N; i++){
            free(adj[i]);
        }
        free(adj);
    }
}

void verifier_vecteur_proba(double* pi, int N, const char* nom){
    double somme = somme_vecteur(pi, N);

    printf("\nSomme %s = %.12f\n", nom, somme);

    if(fabs(somme - 1.0) < EPSILON){ 
        printf("Verification OK : %s est un vecteur de probabilite\n", nom);
    }else{
        printf("Erreur : %s ne somme pas a 1\n", nom);
    }
}

void afficher_graphe(int N, int** adj, int* deg_sortant, const char* titre){
    printf("\n--- %s ---\n", titre);

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
    for(int i = 0; i < N; i++) {
        pi_final[i] = 0.0;
    }

    for(int i = 0; i < N; i++) {
        if(nouveau_id[i] != -1) {
            pi_final[i] = pi_backspace[nouveau_id[i]];
        }
    }

    for(int i = 0; i < nb_impasses; i++) {
        int id_original = impasses[i].id_original;
        int id_copie_start = premiere_copie[id_original];
        int deg_in = impasses[i].degre_entrant;

        double somme_puits = 0.0;
        for(int j = 0; j < deg_in; j++) {
            somme_puits += pi_backspace[id_copie_start + j];
        }
        
        pi_final[id_original] = somme_puits;
    }
}

void comparer_resultats(int N, double* pi_google, double* pi_final, NoeudPuit* impasses, int nb_impasses) {
    printf("\n--- Comparaison des peres d'impasses (Google vs Backspace) ---\n");

    int* est_pere_impasse = calloc(N, sizeof(int));
    if (est_pere_impasse == NULL) {
        printf("Erreur allocation memoire pour est_pere_impasse\n");
        return;
    }

    int nb_peres_uniques = 0;
    for (int i = 0; i < nb_impasses; i++) {
        for (int j = 0; j < impasses[i].degre_entrant; j++) {
            int pere = impasses[i].voisins_entrants[j];
            if (est_pere_impasse[pere] == 0) {
                est_pere_impasse[pere] = 1;
                nb_peres_uniques++;
            }
        }
    }

    printf("Trouve %d peres uniques pointant vers des impasses.\n\n", nb_peres_uniques);
    printf("%-10s | %-18s | %-18s | %-18s\n", "Noeud", "Score Google", "Score Backspace", "Difference (B - G)");
    printf("-----------------------------------------------------------------------\n");

    for (int i = 0; i < N; i++) {
        if (est_pere_impasse[i] == 1) {
            double diff = pi_final[i] - pi_google[i];
            
            printf("%-10d | %.12f   | %.12f   | %+.12f\n", 
                   i, pi_google[i], pi_final[i], diff);
        }
    }

    free(est_pere_impasse);
}

int main(int argc, char* argv[]){

    if (argc < 2) {
        printf("Usage : %s <nom_du_fichier.mtx>\n", argv[0]);
        printf("Exemple : %s test2.mtx\n", argv[0]);
        return 1; 
    }

    srand(time(NULL));

    int N;
    int** adj = NULL;
    int* deg_sortant = NULL;

    int** adj_entrant = NULL;
    int* deg_entrant = NULL;

    NoeudPuit* puits = NULL;
    int nb_puits = 0;

    // 1. Lecture du graphe original
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

    // 2. Si aucun puits, on cree quelques impasses aleatoirement
    if(nb_puits == 0){
        printf("\nAucune impasse detectee : creation aleatoire d'impasses.\n");

        free(deg_entrant);
        free(puits);
        liberer_graphe(adj_entrant, N);

        for(int t = 0; t < 3; t++){
            creer_impasse_aleatoire(N, deg_sortant);
        }

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

    // 3. PageRank Google classique sur le graphe original/modifie
    double* pi_google = malloc(N * sizeof(double));

    if(pi_google == NULL){
        printf("Erreur allocation pi_google\n");
        exit(1);
    }

    printf("\n--- PageRank Google classique ---\n");
    true_pagerank(N, adj, deg_sortant, pi_google);
    afficher_pagerank(N, pi_google);
    verifier_vecteur_proba(pi_google, N, "PageRank Google");

    // 4. Construction du graphe Backspace
    int N2;
    int** adj2 = NULL;
    int* deg_sortant2 = NULL;

    int* nouveau_id = NULL;
    int* premiere_copie = NULL;

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

    printf("\nN = %d, N2 = %d\n", N, N2);

    // 5. PageRank Backspace
    double* pi_backspace = malloc(N2 * sizeof(double));

    if(pi_backspace == NULL){
        printf("Erreur allocation pi_backspace\n");
        exit(1);
    }

    printf("\n--- PageRank Backspace ---\n");
    true_pagerank(N2, adj2, deg_sortant2, pi_backspace);
    // afficher_pagerank(N2, pi_backspace);
    verifier_vecteur_proba(pi_backspace, N2, "PageRank Backspace");

    // 6. Fusion et compare
    double* pi_final = malloc(N * sizeof(double));
    if(pi_final == NULL){
        printf("Erreur allocation pi_final\n");
        exit(1);
    }
    
    fusionner_pagerank(N, pi_backspace, nouveau_id, premiere_copie, puits, nb_puits, pi_final);
    verifier_vecteur_proba(pi_final, N, "PageRank Final (Fusionne)");

    afficher_pagerank(N, pi_final);
    comparer_resultats(N, pi_google, pi_final, puits, nb_puits);

    tracer_nuage_points_gnuplot(N, pi_google, pi_final);

    // 7. Liberation memoire
    free(pi_google);
    free(pi_backspace);
    free(pi_final);
    free(nouveau_id);      
    free(premiere_copie);

    liberer_graphe(adj, N);
    liberer_graphe(adj_entrant, N);
    liberer_graphe(adj2, N2);

    free(deg_sortant);
    free(deg_entrant);
    free(deg_sortant2);
    free(puits);

    return 0;
}