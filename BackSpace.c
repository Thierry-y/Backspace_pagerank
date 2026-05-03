#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id_original;        // ID du nœud dans le graphe original (nœud avec un degré sortant de 0)
    int degre_entrant;      // Son degré entrant (nombre de nœuds pointant vers lui)
    int* voisins_entrants;  // Liste des nœuds entrants (pointe directement vers un tableau dans la liste d'adjacence inverse)
} NoeudPuit;

void creer_impasse_aleatoire(int N, int* deg_sortant){
    int sommet = -1;

    while(sommet == -1){
        int candidat = rand() % N;//choisir un sommet aléatoire, rand() génere un nombre alétoire 
        // très grand puis on le prend modulo N pour rester dans la plage des sommets

        if(deg_sortant[candidat] > 0){
            sommet = candidat;
        }
    }

    //printf("Creation d'une impasse : suppression des sorties du sommet %d\n", sommet);

    deg_sortant[sommet] = 0;
}

void recalculer_impasses(
    int N,
    int** adj,
    int* deg_sortant,
    int*** adj_entrant,
    int** deg_entrant,
    NoeudPuit** puits,
    int* nb_puits
){
    // recalculer deg_entrant
    *deg_entrant = calloc(N, sizeof(int));

    if(*deg_entrant == NULL){
        printf("Erreur allocation deg_entrant\n");
        exit(1);
    }

    for(int i = 0; i < N; i++){
        for(int k = 0; k < deg_sortant[i]; k++){
            //augmenter le nombre de voisins entrant du sommet de destination
            int destination = adj[i][k];
            (*deg_entrant)[destination]++;
        }
    }

    // allouer adj_entrant
    *adj_entrant = malloc(N * sizeof(int*));

    if(*adj_entrant == NULL){
        printf("Erreur allocation adj_entrant\n");
        exit(1);
    }

    for(int i = 0; i < N; i++){
        (*adj_entrant)[i] = malloc((*deg_entrant)[i] * sizeof(int));

        if((*deg_entrant)[i] > 0 && (*adj_entrant)[i] == NULL){
            printf("Erreur allocation adj_entrant[%d]\n", i);
            exit(1);
        }
    }
    //vecteurs des position qui va permettre de connaitre les index courant dans les 
    //liste des voisins entrants et se deplacer correctement
    int* position = calloc(N, sizeof(int));

    if(position == NULL){
        printf("Erreur allocation position\n");
        exit(1);
    }

    // remplir adj_entrant
    for(int i = 0; i < N; i++){
        for(int k = 0; k < deg_sortant[i]; k++){

            int destination = adj[i][k];
            (*adj_entrant)[destination][position[destination]] = i;//ajouter le voisin entrant 
            position[destination]++; //augmenter l'index position
        }
    }

    free(position);

    // compter les impasses
    *nb_puits = 0;

    for(int i = 0; i < N; i++){
        if(deg_sortant[i] == 0 && (*deg_entrant)[i] > 0){
            (*nb_puits)++;
        }
    }

    // allouer le tableau d'impasses
    *puits = malloc((*nb_puits) * sizeof(NoeudPuit));

    if(*nb_puits > 0 && *puits == NULL){
        printf("Erreur allocation puits\n");
        exit(1);
    }

    // remplir le tableau d'impasses
    int index = 0;

    for(int i = 0; i < N; i++){
        if(deg_sortant[i] == 0 && (*deg_entrant)[i] > 0){
            (*puits)[index].id_original = i; //numéro du puit dans le graphe
            (*puits)[index].degre_entrant = (*deg_entrant)[i]; //attribuer au puit le degré entrant
            (*puits)[index].voisins_entrants = (*adj_entrant)[i];  //allouer la liste des voisins entrant du sommet puit
            index++;
        }
    }
}

void lire_et_trouver_impasses(const char* nom_fichier, 
                              int* N, 
                              int*** adj, int** deg_sortant, 
                              int*** adj_entrant, int** deg_entrant,
                              NoeudPuit** puits, int* nb_puits) {
    
    FILE* f = fopen(nom_fichier, "r");
    if(!f){
        printf("Erreur ouverture fichier %s\n", nom_fichier);
        exit(1);
    }

    char ligne[1024];

    if(!fgets(ligne, sizeof(ligne), f))

    {
         exit(1);
    }

    if(strncmp(ligne, "%%MatrixMarket", 14) != 0) { //vérifier que le fichier est en .mtx
        exit(1);
    }

    do {

        if(!fgets(ligne, sizeof(ligne), f)) exit(1);

    } while(ligne[0] == '%');

    int nrows, ncols, M;
    sscanf(ligne, "%d %d %d", &nrows, &ncols, &M);
    *N = nrows;

    int* sources      = malloc(M * sizeof(int)); //allocation de la liste des sommet "source" 
    int* destinations = malloc(M * sizeof(int)); //allocation de la liste des sommet "destination"
    
    *deg_sortant = calloc(*N, sizeof(int));
    *deg_entrant = calloc(*N, sizeof(int));

    for(int k = 0; k < M; k++){
        int u, v;
        fscanf(f, "%d %d", &u, &v);

        u--; v--; // on diminue de 1 la valeur des sommet car dans le .mtx les sommet commence
        //à 1 mais en C les index commencent à 0 dans les tableaux

        sources[k]      = u;
        destinations[k] = v;
        
        (*deg_sortant)[u]++; 
        (*deg_entrant)[v]++; 
    }
    fclose(f);

    *adj        = malloc((*N) * sizeof(int*));
    *adj_entrant = malloc((*N) * sizeof(int*));
    //alocation mémoire des listes des voisins entrants et sortants
    for(int i = 0; i < *N; i++){
        (*adj)[i]         = malloc((*deg_sortant)[i] * sizeof(int));
        (*adj_entrant)[i] = malloc((*deg_entrant)[i] * sizeof(int));
    }
    // allocation des "positions" qui vont permettre de se deplacer dans la liste des voisins sortants et entrants
    int* pos_sortant = calloc(*N, sizeof(int));
    int* pos_entrant = calloc(*N, sizeof(int));

    //remplir les liste d'ajacence "entrante" et "sortante"
    for(int k = 0; k < M; k++){
        int u = sources[k];
        int v = destinations[k];
        
        (*adj)[u][pos_sortant[u]++]         = v;     
        (*adj_entrant)[v][pos_entrant[v]++] = u;  
    }

    free(sources); free(destinations); free(pos_sortant); free(pos_entrant);

    *nb_puits = 0;
    //compter le nombre de puit
    for(int i = 0; i < *N; i++) {
        if((*deg_sortant)[i] == 0 && (*deg_entrant)[i] > 0) {
            (*nb_puits)++;
        }
    }
    //allouer le tableau de puits
    *puits = malloc((*nb_puits) * sizeof(NoeudPuit));

    //remplir le tableau de puits
    int index = 0; //index pour se deplcaer dans le tableau des puits
    for(int i = 0; i < *N; i++) {
        if((*deg_sortant)[i] == 0 && (*deg_entrant)[i] > 0) {
            (*puits)[index].id_original      = i; //stocker le numero du sommet
            (*puits)[index].degre_entrant    = (*deg_entrant)[i];//stocker le degré entrant du sommet
            
            (*puits)[index].voisins_entrants = (*adj_entrant)[i]; //stocker les voisin entrants
            
            index++;
        }
    }

    printf("Lecture terminee: %d noeuds, %d aretes. Trouve %d impasses (dead ends).\n", *N, M, *nb_puits);
}

void construire_graphe_backspace(
    int N,
    int** adj,
    int* deg_sortant,
    NoeudPuit* impasses,
    int nb_impasses,
    int*** adj2,
    int** deg_sortant2,
    int* N2
){

    int nb_copies = 0;

    for(int i = 0; i < nb_impasses; i++){
        nb_copies += impasses[i].degre_entrant; //on ajoute au nombre de copie le nombre de voisins entrants
        //du puits courrant
    }

    *N2 = N - nb_impasses + nb_copies; //nouvelle taille du graphe qui sera des N sommet + le nombre 
    //de copie necessaire pour les puits, on enléve le nombre de puits car il seront renommé par les noms des copies

    int* nouveau_id      = malloc(N * sizeof(int)); //tableau des nouveau id des sommet non puit après "suppressions" des puits
    //au profit des copies
    int* premiere_copie  = malloc(N * sizeof(int)); //tableau qui va contenir les id des première copie des
    //sommet puits, si le sommet 5 est le premier puit, et qu'il a un degré entrant de 5, il aura comme numéro
    //l'entier (x) juste après le dernier sommet non puit du graphe , et la numéroation du  prochain sommet pui
    //commencera a x+5

    if(nouveau_id == NULL || premiere_copie == NULL){
        printf("Erreur allocation mapping\n");
        exit(1);
    }

    for(int i = 0; i < N; i++){
        nouveau_id[i]     = -1; 
        premiere_copie[i] = -1;
    }

    //prochain_id sert à savoir le nom du numéro allouer au prochain sommet
    int prochain_id = 0;

    for(int i = 0; i < N; i++){
        // on cherche si le sommet du graphe est un puit, si c'est un puit on ne fait rien,
        //sinon on lui attribuer le prochain id , de cette facon on renomme les sommets
        //en ne prenant pas en compte les puits qui seront renomé plus tard par le nom
        //des copies, qui commenceron après le numéro du dernier sommet non puit
        int est_impasse = 0;

        for(int j = 0; j < nb_impasses; j++){
            if(impasses[j].id_original == i){
                est_impasse = 1;
            }
        }

        if(est_impasse == 0){
            nouveau_id[i] = prochain_id;
            prochain_id++;
        }
    }

    // donner des ids aux copies des puits
    for(int i = 0; i < nb_impasses; i++){
        // on trouve le numéro du sommet puit original
        int sommet_puit = impasses[i].id_original;

        premiere_copie[sommet_puit] = prochain_id;//Le numéro de la première copie du sommet puis sera le
        //dernier numéro disponible
        prochain_id += impasses[i].degre_entrant;//le prochain id dispo sera à t+ degré entrant du dernier puit
    }

    *deg_sortant2 = calloc(*N2, sizeof(int));//Tableau des nouveau degrés sortant après changements

    if(*deg_sortant2 == NULL){
        printf("Erreur allocation deg_sortant2\n");
        exit(1);
    }

    // calculer deg_sortant2 pour les sommets non puits
    for(int sommet = 0; sommet < N; sommet++){
        if(nouveau_id[sommet] != -1){
            (*deg_sortant2)[nouveau_id[sommet]] = deg_sortant[sommet];//on retrouve le degre sortant original
            //du sommet dans le tableau original des degrés sortants
        }
    }
    //on rajoute un lien sortant de chaque copie des sommet puits vers le sommet père
    for(int i = 0; i < nb_impasses; i++){
        int sommet_puit = impasses[i].id_original;

        for(int j = 0; j < impasses[i].degre_entrant; j++){
            int id_copie = premiere_copie[sommet_puit] + j;
            (*deg_sortant2)[id_copie] = 1;
        }
    }

    // allocation de la nouvelle matrice d'adj
    *adj2 = malloc((*N2) * sizeof(int*));

    if(*adj2 == NULL){
        printf("Erreur allocation adj2\n");
        exit(1);
    }

    for(int i = 0; i < *N2; i++){
        //on alloue en mémoire les liste des voisins sortants
        (*adj2)[i] = malloc((*deg_sortant2)[i] * sizeof(int));

        if((*deg_sortant2)[i] > 0 && (*adj2)[i] == NULL){
            printf("Erreur allocation adj2[%d]\n", i);
            exit(1);
        }
    }

    //  remplir les liens des sommets normaux
    for(int sommet = 0; sommet < N; sommet++){

        if(nouveau_id[sommet] != -1){ //on vérifie que le sommet est non puit(il contient un nouvelle id)

            int id_sommet = nouveau_id[sommet];

            for(int k = 0; k < deg_sortant[sommet]; k++){// on parcours ses voisins
                int destination = adj[sommet][k];

                if(premiere_copie[destination] != -1){// si le noeud de destination est un puit(possède un numéro de premère copie)

                    int indice_impasse = -1; //indice du puit dans le tableau des puits

                    for(int i = 0; i < nb_impasses; i++){
                        if(impasses[i].id_original == destination){//on trouve dans le tableau des puits
                            //le pluit correspondant
                            indice_impasse = i;
                        }
                    }

                    int numero_copie = -1;//on cherche quelle est la bonne copie du puit

                    if(indice_impasse != -1){// on verifie que le puis à bien in indice dans le tab des puits
                        for(int j = 0; j < impasses[indice_impasse].degre_entrant; j++){// on cherche dans
                            //les degré entrants du puit, si il est à l'indice j, alors
                            //le numéro de la copie sera la j ème
                            if(impasses[indice_impasse].voisins_entrants[j] == sommet){
                                numero_copie = j;
                            }
                        }
                    }

                    if(numero_copie == -1){
                        printf("Erreur : copie non trouvee pour %d -> %d\n",
                               sommet, destination);
                        exit(1);
                    }
                    //on affecte à la matrice d'ajacence le bon numero du sommet puit (qui est la destination )
                    (*adj2)[id_sommet][k] = premiere_copie[destination] + numero_copie;
                }
                else{
                    //sinon si le sommet de dest n'est pas puit, on donne simplement le numéro du nouveau sommet
                    //après renommage
                    (*adj2)[id_sommet][k] = nouveau_id[destination];
                }
            }
        }
    }

    //  mettre les liens sortant au sommet puit dans adj2
    for(int i = 0; i < nb_impasses; i++){ // on parcourt les impasses

        int sommet_puit = impasses[i].id_original;//recuperer l'id original du puit

        for(int j = 0; j < impasses[i].degre_entrant; j++){ //parcourt les voisins entrant
            int pere    = impasses[i].voisins_entrants[j]; // recup l'id du père
            int id_copie = premiere_copie[sommet_puit] + j;//trouver l'id de la j ème copie

            (*adj2)[id_copie][0] = nouveau_id[pere];//attribuer l'id nouveau du parent dans 
            //les voisin de la j ème copie du noeud puit
        }
    }

    free(nouveau_id);
    free(premiere_copie);
}

int main(){
    int N;
    int** adj        = NULL;
    int* deg_sortant = NULL;

    int** adj_entrant = NULL;
    int* deg_entrant  = NULL;

    NoeudPuit* puits = NULL;
    int nb_puits     = 0;

    lire_et_trouver_impasses(
        "test.mtx",
        &N,
        &adj,
        &deg_sortant,
        &adj_entrant,
        &deg_entrant,
        &puits,
        &nb_puits
    );

    if(nb_puits == 0){
        free(deg_entrant);
        free(puits);

        if(adj_entrant != NULL){
            for(int i = 0; i < N; i++){
                free(adj_entrant[i]);
            }
            free(adj_entrant);
        }

        creer_impasse_aleatoire(N, deg_sortant);

        adj_entrant = NULL;
        deg_entrant = NULL;
        puits       = NULL;
        nb_puits    = 0;

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

    // Construction du graphe backspace

    int N2;
    int** adj2        = NULL;
    int* deg_sortant2 = NULL;

    construire_graphe_backspace(
        N,
        adj,
        deg_sortant,
        puits,
        nb_puits,
        &adj2,
        &deg_sortant2,
        &N2
    );

    // Affichage du nouveau graphe

    printf("\n--- Graphe Backspace ---\n");

    for(int i = 0; i < N2; i++){
        printf("%d -> ", i);

        for(int k = 0; k < deg_sortant2[i]; k++){
            printf("%d ", adj2[i][k]);
        }

        printf("\n");
    }

    printf("\nN = %d, N2 = %d\n", N, N2);
    // Libération mémoire

    for(int i = 0; i < N; i++){
        free(adj[i]);
        free(adj_entrant[i]);
    }

    free(adj);
    free(adj_entrant);
    free(deg_sortant);
    free(deg_entrant);

    for(int i = 0; i < N2; i++){
        free(adj2[i]);
    }

    free(adj2);
    free(deg_sortant2);
    free(puits);

    return 0;
}