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

    if(!fgets(ligne, sizeof(ligne), f)) exit(1);
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

    int* pos_out = calloc(*N, sizeof(int));
    int* pos_in  = calloc(*N, sizeof(int));

    for(int k = 0; k < M; k++){
        int u = src[k];
        int v = dst[k];
        
        (*adj)[u][pos_out[u]++] = v;     
        (*adj_in)[v][pos_in[v]++] = u;  
    }

    free(src); free(dst); free(pos_out); free(pos_in);

    *num_dead_ends = 0;
    
    for(int i = 0; i < *N; i++) {
        if((*deg_out)[i] == 0 && (*deg_in)[i] > 0) {
            (*num_dead_ends)++;
        }
    }

    *dead_ends = malloc((*num_dead_ends) * sizeof(DeadEndNode));

    int index = 0;
    for(int i = 0; i < *N; i++) {
        if((*deg_out)[i] == 0 && (*deg_in)[i] > 0) {
            (*dead_ends)[index].original_id = i;
            (*dead_ends)[index].in_degree = (*deg_in)[i];
            
            (*dead_ends)[index].incoming_nodes = (*adj_in)[i]; 
            
            index++;
        }
    }

    printf("Lecture terminee: %d noeuds, %d aretes. Trouve %d impasses (dead ends).\n", *N, M, *num_dead_ends);
}
