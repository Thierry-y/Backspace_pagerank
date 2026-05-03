#ifndef BACKSPACE_H
#define BACKSPACE_H


typedef struct {
    int id_original;        // ID du nœud dans le graphe original (nœud avec un degré sortant de 0)
    int degre_entrant;      // Son degré entrant (nombre de nœuds pointant vers lui)
    int* voisins_entrants;  // Liste des nœuds entrants (pointe directement vers un tableau dans la liste d'adjacence inverse)
} NoeudPuit;

void lire_et_trouver_impasses(
    const char* nom_fichier,
    int* N,
    int*** adj,
    int** deg_sortant,
    int*** adj_entrant,
    int** deg_entrant,
    NoeudPuit** puits,
    int* nb_puits
);

void creer_impasse_aleatoire(int N, int* deg_sortant);

void recalculer_impasses(
    int N,
    int** adj,
    int* deg_sortant,
    int*** adj_entrant,
    int** deg_entrant,
    NoeudPuit** puits,
    int* nb_puits
);

void construire_graphe_backspace(
    int N,
    int** adj,
    int* deg_sortant,
    NoeudPuit* puits,
    int nb_puits,
    int*** adj2,
    int** deg_sortant2,
    int* N2
);

#endif