#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "PageRank.h"
#define EPSILON 1e-6
#define ALPHA 0.85

// Utilitaires
//Calculer la norme L1 entre 2 vecteur
double norme_L1(double* a, double* b, int N){
    double s = 0.0;
    for(int i=0;i<N;i++)
        s += fabs(a[i]-b[i]);
    return s;
}
//Allouer la matrice
int** creer_matrice_int(int N){
    int** mat = malloc(N * sizeof(int*));
    for(int i=0;i<N;i++)
        mat[i] = NULL;
    return mat;
}

void liberer_matrice(int** mat, int N){
    for(int i=0;i<N;i++)
        if(mat[i]) free(mat[i]);
    free(mat);
}
// lecture d'un graphe sous format matrix planet
void lire_graphe_matrixmarket(const char* nom_fichier, int* N, int*** adj, int** deg_out){
    FILE* f = fopen(nom_fichier, "r");
    if(!f){
        printf("Erreur dans l'ouverture fichier %s\n", nom_fichier);
        exit(1);
    }

    char ligne[1024];

    // Lire l'en-tête Matrix Market
    if(!fgets(ligne, sizeof(ligne), f)){
        printf("Erreur lecture en-tete\n");
        fclose(f);
        exit(1);
    }
    //vérifier que le fichier est un .mtx
    if(strncmp(ligne, "%%MatrixMarket", 14) != 0){
        printf("Le fichier n'est pas au format MatrixMarket!!\n");
        fclose(f);
        exit(1);
    }

    // Sauter les lignes de commentaires (qui commencent par %)
    do {
        if(!fgets(ligne, sizeof(ligne), f)){
            printf("Erreur lecture dimensions\n");
            fclose(f);
            exit(1);
        }
    } while(ligne[0] == '%');

    int nrows, ncols, M;
    if(sscanf(ligne, "%d %d %d", &nrows, &ncols, &M) != 3){
        printf("Erreur lecture dimensions MatrixMarket\n");
        fclose(f);
        exit(1);
    }

    if(nrows != ncols){
        printf("Le graphe doit etre carre pour le PageRank (%d x %d)\n", nrows, ncols);
        fclose(f);
        exit(1);
    }

    *N = nrows;

    // Temporairement stocker toutes les aretes
    int* src = malloc(M * sizeof(int));
    int* dst = malloc(M * sizeof(int));
    *deg_out = calloc(*N, sizeof(int));

    if(src == NULL || dst == NULL || *deg_out == NULL)
        {
        printf("Erreur une des allocation mémoire\n");
        fclose(f);
        exit(1);
    }

    // Lecture des (i,j)
    // Convention choisie ici :
    // une ligne "i j" signifie un arc i -> j
    // donc source = i-1, destination = j-1
    for(int k = 0; k < M; k++){
        int i, j;
        if(fscanf(f, "%d %d", &i, &j) != 2){
            printf("Erreur dans la lecture de l'arete numero %d\n", k);
            fclose(f);
            exit(1);
        }

        i--;  // passage en base 0 pour faciliter les calculs
        j--;

        if(i < 0 || i >= *N || j < 0 || j >= *N){ // on vérifie que les indice i et j reste
            //dans les bornes des valeurs du graphe
            printf("Indice hors bornes dans le fichier : %d %d\n", i+1, j+1);
            fclose(f);
            exit(1);
        }

        src[k] = i;
        dst[k] = j;
        (*deg_out)[i]++; // on augmente le degré sortant
    }

    fclose(f);

    // Allocation de la liste d'adjacence
    *adj = malloc((*N) * sizeof(int*));
    if((*adj)==NULL){
        printf("Erreur lors de l'allocation de adj\n");
        exit(1);
    }

    for(int i = 0; i < *N; i++){
        (*adj)[i] = malloc((*deg_out)[i] * sizeof(int));//On aloue dynamiquement
        //la liste des voisin du sommet i
        if((*deg_out)[i] > 0 && (*adj)[i]==NULL){
            printf("Erreur dans l'allocation de adj[%d]\n", i);
            exit(1);
        }
    }

    int* pos = calloc(*N, sizeof(int));//tableau des positions qui 
    //permet de se deplacer dans les tableau des voisins de adj
    if(pos==NULL){
        printf("Erreur dans l'allocation de pos\n");
        exit(1);
    }
    //On remplit les liste de voisins des sommets
    for(int k = 0; k < M; k++){
        int u = src[k];
        int v = dst[k];
        (*adj)[u][pos[u]] = v; 
        pos[u] = pos[u] + 1;// on met à jour l'index
    }

    free(src);
    free(dst);
    free(pos);
}
/*
// Lecture graphe matrice complète (pas utilisé dans le projet car on utilise une repr en matrice creuse avec liste d'adjacence)
void lire_graphe_matrice(const char* nom_fichier, int* N, int*** adj_matrix, int** deg_out){
    FILE* f = fopen(nom_fichier,"r");
    if(!f){ printf("Erreur ouverture fichier\n"); exit(1); }

    fscanf(f,"%d", N);
    *adj_matrix = creer_matrice_int(*N);
    *deg_out = malloc((*N) * sizeof(int));

    for(int i=0;i<*N;i++){
        (*adj_matrix)[i] = malloc((*N)*sizeof(int));
        (*deg_out)[i] = 0;
        for(int j=0;j<*N;j++){
            fscanf(f,"%d", &(*adj_matrix)[i][j]);
            if((*adj_matrix)[i][j]==1) (*deg_out)[i]++;
        }
    }
    fclose(f);
}
*/
// PageRank liste d’adjacence (sans le surfeur aléatoire)
void pagerank_liste(int N, int** adj, int* deg_out, double* pi){
    double* pi_new = malloc(N*sizeof(double));
    for(int i=0;i<N;i++)
    {
         pi[i]=1.0/N;
    }
    double diff;
    int iter=0;

    do{
        
        for(int i=0;i<N;i++){
             pi_new[i]=0.0;
        }     
        for(int i=0;i<N;i++){
           if(deg_out[i]==0){ // si le sommet est un puit, on ajoute à tout les autre sommet(et lui) le score du pui
            //diviser par N
                for(int j=0;j<N;j++) {
                    pi_new[j]+=pi[i]/N;
                }
            } else {
                for(int k=0;k<deg_out[i];k++){ // si il n'est pas puit on donne à tout les sommets(voisins sortants) le score
                    //du sommet diviser par le degré du sommet
                    int j=adj[i][k];
                    pi_new[j]+=pi[i]/deg_out[i];
                }
                }
        }

        diff = norme_L1(pi,pi_new,N);
        for(int i=0;i<N;i++) pi[i]=pi_new[i];
        iter++;
    } while(diff>EPSILON);

    printf("Convergence en %d iterations\n", iter);
}
//pagerank avec surfer aléatoire
void true_pagerank(int N, int** adj, int* deg_out, double* x_pair ){
    double* x_impair = malloc(N*sizeof(double));

    // Initialisation, toutes les pages on le même score au départ
    for(int i=0;i<N;i++) 
        x_pair[i] = 1.0/N;

    int iter = 0;
    double diff;

    // vecteur f 
    //double * f = malloc(N*sizeof(double));

    do{
        // On initialise le nouveau vecteur avec la partie téléportation.
        // Le surfeur peut aller sur n'importe quelle page avec probabilité 1 - ALPHA.
        for(int i=0;i<N;i++) 
            x_impair[i] = (1.0 - ALPHA) / N;

        // xf représente la masse de probabilité portée par les puits,
        // c'est-à-dire les sommets qui n'ont aucun arc sortant.
        double xf = 0.0;
        for(int i=0;i<N;i++){
            if(deg_out[i]==0){
               // f[i] = 1;
                xf += x_pair[i];
            }//else{
              //  f[i] = 0;
            //}
        }

        // La masse des puits est redistribuée uniformément à toutes les pages.
        // On la multiplie par ALPHA car elle fait partie de la navigation par liens.
        for(int j=0;j<N;j++){
            x_impair[j] += ALPHA * xf / N;
        }

        // Pour les sommets qui ont des arcs sortants,
        // on répartit leur score entre leurs voisins.
        for(int i=0;i<N;i++){
            if(deg_out[i] > 0){
                for(int k=0;k<deg_out[i];k++){
                    int j = adj[i][k];
                    // Le sommet i donne une part égale de son score
                    // à chacun de ses voisins sortants.
                    x_impair[j] += ALPHA * x_pair[i] / deg_out[i];
                }
            }
        }

        diff = norme_L1(x_pair, x_impair, N);

        for(int i=0;i<N;i++) 
            x_pair[i] = x_impair[i];

        iter++;

    }while(diff > EPSILON);

    printf("True PageRank a convergé en %d iterations\n", iter);

    free(x_impair);
   // free(f);
}
//calcule la somme des elements d'un vecteur, afin de pouvoir vérifier que 
//les vecteur renvoyés sont des vecteurs de probas
double somme_vecteur(double* v, int N){
    double s = 0.0;

    for(int i = 0; i < N; i++){
        s += v[i];
    }

    return s;
}

// Affichage des pages et de leurs scores

void afficher_pagerank(int N, double* pi){
    for(int i=0;i<N;i++)
        printf("Page %d : %.8f\n", i, pi[i]);

}

