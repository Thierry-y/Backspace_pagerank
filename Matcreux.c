#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define EPSILON 1e-6
#define ALPHA 0.85
// ========================
// Utilitaires
// ========================
double norme_L1(double* a, double* b, int N){
    double s = 0.0;
    for(int i=0;i<N;i++)
        s += fabs(a[i]-b[i]);
    return s;
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
        printf("Erreur ouverture fichier %s\n", nom_fichier);
        exit(1);
    }

    char ligne[1024];

    // Lire l'en-tête Matrix Market
    if(!fgets(ligne, sizeof(ligne), f)){
        printf("Erreur lecture en-tete\n");
        fclose(f);
        exit(1);
    }

    if(strncmp(ligne, "%%MatrixMarket", 14) != 0){
        printf("Le fichier n'est pas au format MatrixMarket\n");
        fclose(f);
        exit(1);
    }

    // Sauter les lignes de commentaires
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

    if(!src || !dst || !(*deg_out)){
        printf("Erreur allocation memoire\n");
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
            printf("Erreur lecture arete %d\n", k);
            fclose(f);
            exit(1);
        }

        i--;  // passage en base 0
        j--;

        if(i < 0 || i >= *N || j < 0 || j >= *N){
            printf("Indice hors bornes dans le fichier : %d %d\n", i+1, j+1);
            fclose(f);
            exit(1);
        }

        src[k] = i;
        dst[k] = j;
        (*deg_out)[i]++;
    }

    fclose(f);

    // Allocation de la liste d'adjacence
    *adj = malloc((*N) * sizeof(int*));
    if(!(*adj)){
        printf("Erreur allocation adj\n");
        exit(1);
    }

    for(int i = 0; i < *N; i++){
        (*adj)[i] = malloc((*deg_out)[i] * sizeof(int));
        if((*deg_out)[i] > 0 && !(*adj)[i]){
            printf("Erreur allocation adj[%d]\n", i);
            exit(1);
        }
    }

    // Remplissage
    int* pos = calloc(*N, sizeof(int));
    if(!pos){
        printf("Erreur allocation pos\n");
        exit(1);
    }

    for(int k = 0; k < M; k++){
        int u = src[k];
        int v = dst[k];
        (*adj)[u][pos[u]++] = v;
    }

    free(src);
    free(dst);
    free(pos);
}
/*lire un graphe sous le format suivant :

6 14
4 0 1 3 5
4 2 3 4 5
4 0 1 3 4
1 4
1 2
0
*/


void lire_graphe_liste(const char* nom_fichier, int* N, int*** adj, int** deg_out){
    FILE* f = fopen(nom_fichier,"r");
    if(!f){ printf("Erreur ouverture fichier\n"); exit(1); }

    int M;
    fscanf(f,"%d %d", N, &M);  // N sommets, M arcs (optionnel)
    *deg_out = malloc((*N) * sizeof(int));
    *adj = malloc((*N) * sizeof(int*));

    for(int i=0;i<*N;i++){
        int d;
        fscanf(f,"%d",&d);           // nombre d'arcs sortants
        (*deg_out)[i] = d;
        (*adj)[i] = malloc(d * sizeof(int));
        for(int j=0;j<d;j++)
            fscanf(f,"%d",&(*adj)[i][j]);
    }

    fclose(f);
}

// ========================
// PageRank liste d’adjacence
// ========================
void pagerank_liste(int N, int** adj, int* deg_out, double* pi){
    double* pi_new = malloc(N*sizeof(double));
   // pi=malloc(N*sizeof(double));
    for(int i=0;i<N;i++) pi[i]=1.0/N;

    double diff;
    int iter=0;

    do{
        
        for(int i=0;i<N;i++) pi_new[i]=0.0;
        for(int i=0;i<N;i++){
           if(deg_out[i]==0){
                for(int j=0;j<N;j++) pi_new[j]+=pi[i]/N;
            } else {
                for(int k=0;k<deg_out[i];k++){
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
void true_pagerank_2(int N, int** adj, int* deg_out, double* x_pair ){
    double* x_impair = malloc(N*sizeof(double));

    // Initialisation
    for(int i=0;i<N;i++) 
        x_pair[i] = 1.0/N;

    int iter = 0;
    double diff;

    // vecteur f (dangling nodes)
    double * f = malloc(N*sizeof(double));

    do{
        // reset
        for(int i=0;i<N;i++) 
            x_impair[i] = (1.0 - ALPHA) / N;

        // construire f et calculer xf
        double xf = 0.0;
        for(int i=0;i<N;i++){
            if(deg_out[i]==0){
                f[i] = 1;
                xf += x_pair[i];
            }else{
                f[i] = 0;
            }
        }

        // redistribution des dangling nodes
        for(int j=0;j<N;j++){
            x_impair[j] += ALPHA * xf / N;
        }

        // propagation via les arcs
        for(int i=0;i<N;i++){
            if(deg_out[i] > 0){
                for(int k=0;k<deg_out[i];k++){
                    int j = adj[i][k];
                    x_impair[j] += ALPHA * x_pair[i] / deg_out[i];
                }
            }
        }

        diff = norme_L1(x_pair, x_impair, N);

        for(int i=0;i<N;i++) 
            x_pair[i] = x_impair[i];

        iter++;

    }while(diff > EPSILON);

    printf("True PageRank convergence en %d iterations\n", iter);

    free(x_impair);
    free(f);
}

// ========================
// Affichage
// ========================
void afficher_pagerank(int N, double* pi){
    for(int i=0;i<N;i++)
        printf("Page %d : %.8f\n", i, pi[i]);

}

// ========================
// Main
// ========================
int main(){
    int N;
    int** adj = NULL;
    int* deg_out = NULL;
    double* pi = NULL;

    //lire_graphe_liste("matCreux.txt", &N, &adj, &deg_out);
    lire_graphe_matrixmarket("wikipedia.mtx", &N, &adj, &deg_out);
    pi = malloc(N*sizeof(double));
    true_pagerank_2(N, adj, deg_out, pi);
    //pagerank_liste(N, adj, deg_out, pi);
    //afficher_pagerank(N, pi);

    liberer_matrice(adj, N);
    free(deg_out);
    free(pi);

    return 0;
}