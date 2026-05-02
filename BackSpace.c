#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int original_id;     // ID du nœud dans le graphe original (nœud avec un degré sortant de 0)
    int in_degree;       // Son degré entrant (nombre de nœuds pointant vers lui)
    int* incoming_nodes; // Liste des nœuds entrants (pointe directement vers un tableau dans la liste d'adjacence inverse)
} DeadEndNode;

void lire_et_trouver_impasses(const char* nom_fichier, 
                              int* N, 
                              int*** adj, int** deg_out, 
                              int*** adj_in, int** deg_in,
                              DeadEndNode** dead_ends, int* num_dead_ends) {
    
    FILE* f = fopen(nom_fichier, "r");
    if(!f){
        printf("Erreur ouverture fichier %s\n", nom_fichier);
        exit(1);
    }

    char ligne[1024];

    if(!fgets(ligne, sizeof(ligne), f))
    { exit(1);
    }
    if(strncmp(ligne, "%%MatrixMarket", 14) != 0) exit(1);
    do {
        if(!fgets(ligne, sizeof(ligne), f)) exit(1);
    } while(ligne[0] == '%');

    int nrows, ncols, M;
    sscanf(ligne, "%d %d %d", &nrows, &ncols, &M);
    *N = nrows;

    int* src = malloc(M * sizeof(int));
    int* dst = malloc(M * sizeof(int));
    
    *deg_out = calloc(*N, sizeof(int));
    *deg_in  = calloc(*N, sizeof(int));

    for(int k = 0; k < M; k++){
        int u, v;
        fscanf(f, "%d %d", &u, &v);
        u--; v--; 

        src[k] = u;
        dst[k] = v;
        
        (*deg_out)[u]++; 
        (*deg_in)[v]++; 
    }
    fclose(f);

    *adj    = malloc((*N) * sizeof(int*));
    *adj_in = malloc((*N) * sizeof(int*));

    for(int i = 0; i < *N; i++){
        (*adj)[i]    = malloc((*deg_out)[i] * sizeof(int));
        (*adj_in)[i] = malloc((*deg_in)[i] * sizeof(int));
    }
    // allocation des "positions" qui vont permettre de se deplacer dans la liste des voisins sortants et entrants
    int* pos_out = calloc(*N, sizeof(int));
    int* pos_in  = calloc(*N, sizeof(int));
    //remplir les liste d'ajacence "entrante" et "sortante"
    for(int k = 0; k < M; k++){
        int u = src[k];
        int v = dst[k];
        
        (*adj)[u][pos_out[u]++] = v;     
        (*adj_in)[v][pos_in[v]++] = u;  
    }

    free(src); free(dst); free(pos_out); free(pos_in);

    *num_dead_ends = 0;
    //compter le nombre de puit
    for(int i = 0; i < *N; i++) {
        if((*deg_out)[i] == 0 && (*deg_in)[i] > 0) {
            (*num_dead_ends)++;
        }
    }
    //allouer le tableau de puits
    *dead_ends = malloc((*num_dead_ends) * sizeof(DeadEndNode));

    //remplir le tableau de puits
    int index = 0; //index pour se deplcaer dans le tableau des puits
    for(int i = 0; i < *N; i++) {
        if((*deg_out)[i] == 0 && (*deg_in)[i] > 0) {
            (*dead_ends)[index].original_id = i; //stocker le numero du sommet
            (*dead_ends)[index].in_degree = (*deg_in)[i];//stocker le degré entrant du sommet
            
            (*dead_ends)[index].incoming_nodes = (*adj_in)[i]; //stocker les voisin entrants
            
            index++;
        }
    }

    printf("Lecture terminee: %d noeuds, %d aretes. Trouve %d impasses (dead ends).\n", *N, M, *num_dead_ends);
}
void construire_graphe_backspace(
    int N,
    int** adj,
    int* deg_out,
    DeadEndNode* impasses,
    int nb_impasses,
    int*** adj2,
    int** deg_out2,
    int* N2
){
    int nb_copies = 0;

    for(int i = 0; i < nb_impasses; i++){
        nb_copies += impasses[i].in_degree;
    }

    *N2 = N - nb_impasses + nb_copies;

    int* nouveau_id = malloc(N * sizeof(int));
    int* premiere_copie = malloc(N * sizeof(int));

    if(nouveau_id == NULL || premiere_copie == NULL){
        printf("Erreur allocation mapping\n");
        exit(1);
    }

    for(int i = 0; i < N; i++){
        nouveau_id[i] = -1;
        premiere_copie[i] = -1;
    }

    // 1) donner un nouvel id aux sommets NON impasses
    int prochain_id = 0;

    for(int i = 0; i < N; i++){
        int est_impasse = 0;

        for(int j = 0; j < nb_impasses; j++){
            if(impasses[j].original_id == i){
                est_impasse = 1;
            }
        }

        if(est_impasse == 0){
            nouveau_id[i] = prochain_id;
            prochain_id++;
        }
    }

    // 2) donner des ids aux copies des impasses
    for(int i = 0; i < nb_impasses; i++){
        int sommet_puit = impasses[i].original_id;

        premiere_copie[sommet_puit] = prochain_id;
        prochain_id += impasses[i].in_degree;
    }

    *deg_out2 = calloc(*N2, sizeof(int));

    if(*deg_out2 == NULL){
        printf("Erreur allocation deg_out2\n");
        exit(1);
    }

    // 3) calculer deg_out2 pour les sommets normaux
    for(int sommet = 0; sommet < N; sommet++){
        if(nouveau_id[sommet] != -1){
            (*deg_out2)[nouveau_id[sommet]] = deg_out[sommet];
        }
    }

    // 4) chaque copie a une sortie vers son parent
    for(int i = 0; i < nb_impasses; i++){
        int sommet_puit = impasses[i].original_id;

        for(int j = 0; j < impasses[i].in_degree; j++){
            int id_copie = premiere_copie[sommet_puit] + j;
            (*deg_out2)[id_copie] = 1;
        }
    }

    // 5) allocation adj2
    *adj2 = malloc((*N2) * sizeof(int*));

    if(*adj2 == NULL){
        printf("Erreur allocation adj2\n");
        exit(1);
    }

    for(int i = 0; i < *N2; i++){
        (*adj2)[i] = malloc((*deg_out2)[i] * sizeof(int));

        if((*deg_out2)[i] > 0 && (*adj2)[i] == NULL){
            printf("Erreur allocation adj2[%d]\n", i);
            exit(1);
        }
    }

    // 6) remplir les liens des sommets normaux
    for(int sommet = 0; sommet < N; sommet++){
        if(nouveau_id[sommet] != -1){

            int id_sommet = nouveau_id[sommet];

            for(int k = 0; k < deg_out[sommet]; k++){
                int destination = adj[sommet][k];

                if(premiere_copie[destination] != -1){
                    int indice_impasse = -1;

                    for(int i = 0; i < nb_impasses; i++){
                        if(impasses[i].original_id == destination){
                            indice_impasse = i;
                        }
                    }

                    int numero_copie = -1;

                    if(indice_impasse != -1){
                        for(int j = 0; j < impasses[indice_impasse].in_degree; j++){
                            if(impasses[indice_impasse].incoming_nodes[j] == sommet){
                                numero_copie = j;
                            }
                        }
                    }

                    if(numero_copie == -1){
                        printf("Erreur : copie non trouvee pour %d -> %d\n",
                               sommet, destination);
                        exit(1);
                    }

                    (*adj2)[id_sommet][k] = premiere_copie[destination] + numero_copie;
                }
                else{
                    (*adj2)[id_sommet][k] = nouveau_id[destination];
                }
            }
        }
    }

    // 7) remplir les copies : retour vers le parent
    for(int i = 0; i < nb_impasses; i++){
        int sommet_puit = impasses[i].original_id;

        for(int j = 0; j < impasses[i].in_degree; j++){
            int parent = impasses[i].incoming_nodes[j];
            int id_copie = premiere_copie[sommet_puit] + j;

            (*adj2)[id_copie][0] = nouveau_id[parent];
        }
    }

    free(nouveau_id);
    free(premiere_copie);
}

int main(){
    int N;
    int** adj = NULL;
    int* deg_out = NULL;

    int** adj_in = NULL;
    int* deg_in = NULL;

    DeadEndNode* dead_ends = NULL;
    int num_dead_ends = 0;

    lire_et_trouver_impasses(
        "test.mtx",
        &N,
        &adj,
        &deg_out,
        &adj_in,
        &deg_in,
        &dead_ends,
        &num_dead_ends
    );
    // Construction du graphe backspace

    int N2;
    int** adj2 = NULL;
    int* deg_out2 = NULL;

    construire_graphe_backspace(
        N,
        adj,
        deg_out,
        dead_ends,
        num_dead_ends,
        &adj2,
        &deg_out2,
        &N2
    );

    // Affichage du nouveau graphe

    printf("\n--- Graphe Backspace ---\n");

    for(int i = 0; i < N2; i++){
        printf("%d -> ", i);

        for(int k = 0; k < deg_out2[i]; k++){
            printf("%d ", adj2[i][k]);
        }

        printf("\n");
    }

    printf("\nN = %d, N2 = %d\n", N, N2);
    // Libération mémoire

    for(int i = 0; i < N; i++){
        free(adj[i]);
        free(adj_in[i]);
    }

    free(adj);
    free(adj_in);
    free(deg_out);
    free(deg_in);

    for(int i = 0; i < N2; i++){
        free(adj2[i]);
    }

    free(adj2);
    free(deg_out2);
    free(dead_ends);

    return 0;
}