#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "BackSpace.h"
#include "PageRank.h"

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

void comparer_resultats(int N, double* pi_google, double* pi_final) {
    double diff_max = 0.0;
    int noeud_diff_max = -1;
    
    printf("\n--- Comparaison des scores (Google vs Backspace) ---\n");
    for(int i = 0; i < N; i++) {
        double diff = fabs(pi_google[i] - pi_final[i]);
        if(diff > diff_max) {
            diff_max = diff;
            noeud_diff_max = i;
        }
    }
    
    if (noeud_diff_max != -1) {
        printf("La plus grande difference est sur le noeud %d : diff = %.8f\n", noeud_diff_max, diff_max);
        printf("   Score Google    = %.8f\n", pi_google[noeud_diff_max]);
        printf("   Score Backspace = %.8f\n", pi_final[noeud_diff_max]);
    }
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
    // afficher_pagerank(N, pi_google);
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

    // afficher_pagerank(N, pi_final);
    comparer_resultats(N, pi_google, pi_final);

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