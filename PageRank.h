#ifndef PAGERANK_H
#define PAGERANK_H

#define EPSILON 1e-6
#define ALPHA 0.85

double norme_L1(double* a, double* b, int N);

void true_pagerank(
    int N,
    int** adj,
    int* deg_sortant,
    double* pi
);

double somme_vecteur(double* v, int N);

void afficher_pagerank(int N, double* pi);

#endif