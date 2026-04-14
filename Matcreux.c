#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define EPSILON 1e-6
#define ALPHA 0.85

// Utilitaires
double norme_L1(double* a, double* b, int N){
    double s = 0.0;
    for(int i=0;i<N;i++)
        s += fabs(a[i]-b[i]);
    return s;
}
//Allouer la matrice pour pagerank avec matrice complete
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
void lire_graphe_matrix_market(const char* nom_fichier, int* N, int*** adj, int** deg_out){
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
    /**/
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

// Lecture graphe matrice complète
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

// PageRank liste d’adjacence
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
void true_pagerank(int N, int** adj, int* deg_out, double* x_pair ){
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

        // redistribution des noeud "puis"
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

// Affichage
void afficher_pagerank(int N, double* pi){
    for(int i=0;i<N;i++)
        printf("Page %d : %.8f\n", i, pi[i]);

}
// Verifier que un vecteur est un vecteur de proba
double verrifier_pi(double *pi,int N){
    double somme=0;
    for(int i=0;i<N;i++){
        somme+=pi[i];
    }
    return somme;
}

// Main
int main(){
    
    int N;
    int** adj = NULL;
    int* deg_out = NULL;
    double* pi = NULL;

    //lire_graphe_liste("matCreux.txt", &N, &adj, &deg_out);
    lire_graphe_matrix_market("wikipedia.mtx", &N, &adj, &deg_out);
    pi = malloc(N*sizeof(double));
    true_pagerank(N, adj, deg_out, pi);
    //pagerank_liste(N, adj, deg_out, pi);
    //afficher_pagerank(N, pi);

    liberer_matrice(adj, N);
    double verrification_pi =verrifier_pi(pi,N);
    if(verrification_pi==1.000000){
        printf("Le vecteur pi est valable");
    }else{
        printf("Le vecteur pi n'est pas valable, sa somme vaut %2f ",verrification_pi);
    }
    free(deg_out);
    free(pi);

    return 0;
    /* exemple d'affichage de courbe avec gnuplot
        FILE *gnuplot;

    // Ouvre Gnuplot en écriture
    gnuplot = popen("gnuplot -persistent", "w");

    if (gnuplot == NULL) {
        printf("Erreur ouverture Gnuplot\n");
        return 1;
    }

    // Commandes envoyées à Gnuplot
    fprintf(gnuplot, "set title 'Courbe y = x^2'\n");
    fprintf(gnuplot, "plot x**2\n");

    // Ferme le processus
    pclose(gnuplot);

    return 0;*/
}