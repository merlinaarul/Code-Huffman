#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

typedef struct noeud
{
	struct noeud *droite; //fils droit
	struct noeud *gauche; //fils gauche
	struct noeud *next; //noeud suivant
	uint32_t poids;
	char symbole;
	_Bool est_feuille; 
}noeud;

typedef struct
{
	noeud *tete;
}arbre;

typedef struct
{
	uint32_t code;
	uint8_t taille;
}code_huff;

typedef struct 
{
    uint8_t buffer;  // Contient les bits avant écriture
    uint32_t nb_bit;   // Nombre de bits stockés dans le buffer
}buffer_binaire;

code_huff table_huffman[256]; 
/*------------------------------------------------------------------*/

uint32_t *compter_occurrence(FILE *f);
arbre *ajouter_noeud(FILE *f);
arbre *trier_pile(arbre *p);
arbre *creerArbre();
noeud *creerNoeud(uint32_t poids, char symbole, _Bool est_feuille);
void push(arbre *p, noeud *n);
void ajouter(arbre *p, noeud *n);
noeud *pop(arbre *p);
arbre *construire_arbre(arbre *arb);
_Bool est_gauche(noeud *n);
_Bool est_droite(noeud *n);
void parcourir_noeud(noeud *n, uint32_t code_actuel, uint32_t profondeur);
void liberer_arbre(noeud *racine);
void ajouter_bit(buffer_binaire *buf, uint8_t bit, FILE *out);
void vider_buffer(buffer_binaire *buf, FILE *out);
void encoder(FILE *in, FILE *out);
void ecrire_entete(FILE *out, uint32_t total_bit);

/*------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	FILE *f = fopen(argv[1], "rb");
	arbre *a = creerArbre();
	if (f == NULL)
	{
    	perror("Erreur lors de l'ouverture du fichier");
    	return EXIT_FAILURE;
    }
    
    liberer_arbre(a->tete); 
	free(a);
	a = trier_pile(ajouter_noeud(f));
	a = construire_arbre(a);
    parcourir_noeud(a->tete, 0, 0);
    
    FILE *out = fopen(argv[2], "wb");
    if (out == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        fclose(f);
   		liberer_arbre(a->tete);
		free(a);
        return EXIT_FAILURE;
    }
    
    rewind(f);
    encoder(f, out);
    fclose(f);
    fclose(out);
    liberer_arbre(a->tete);
	free(a);
	return EXIT_SUCCESS;
}
/*--------------------------COMPRESSION-----------------------------*/	
arbre *creerArbre()
{
	arbre *a = malloc(sizeof(arbre));
	if(a == NULL)
	{
		fprintf(stderr, "Erreur d'allocation mémoire\n");
		exit(EXIT_FAILURE);
	}
	a->tete = NULL;
	return a;
}

noeud *creerNoeud(uint32_t poids, char symbole, _Bool est_feuille)
{
	noeud *n = malloc(sizeof(noeud));
	if(n == NULL)
	{
		fprintf(stderr, "Erreur d'allocation mémoire\n");
		exit(EXIT_FAILURE);
	}
	n->droite = NULL;
	n->gauche = NULL;
	n->next = NULL;
	n->poids = poids;
	n->symbole = symbole;
	n->est_feuille = est_feuille;
	return n;
}

	
uint32_t *compter_occurrence(FILE *f)
{
	char c;
	uint32_t nb = 256;
	//tab qui pourra contenir 256 cases de 32 bits
	uint32_t *tab = (uint32_t *)malloc(nb*sizeof(uint32_t));
	uint32_t e = fscanf(f, "%c", &c);
	if(tab == NULL)
	{
		fprintf(stderr, "L'allocation méoire a échoue\n");
		exit(EXIT_FAILURE);
	}
	for(uint32_t i=0; i<nb; i++)
	{
		tab[i] = 0;
	}
	while(e != EOF)
	{
		 uint32_t i = (uint32_t)(unsigned char)c; //permet de convertir char en ascii
		 tab[i] = tab[i] + 1;
		 e = fscanf(f, "%c", &c);
	}
	return(tab);
}

void push(arbre *p, noeud *n)
{
	if (n == NULL)
    {
        printf(" ERREUR : Impossible de créer un nœud !\n");
        return;
    }
	n->next = p->tete;
	p->tete = n;
	
}

noeud *pop(arbre *p)
{
	if (p->tete == NULL)
    {
        fprintf(stderr, "Erreur : tentative de pop sur une pile vide\n");
        exit(EXIT_FAILURE);
    }
    noeud *n = p->tete;
    p->tete = p->tete->next;

	return n;
}


arbre *trier_pile(arbre *p)
{
	arbre *p2 = creerArbre();
	while(p->tete != NULL)
	{
		noeud *temp = pop(p); // retire l'élément en tete de p
		while(p2->tete != NULL && (temp->poids > p2->tete->poids))
		{
			push(p, pop(p2)); //on retire l'element en tete de p2
		}
		push(p2, temp);
	}
	free(p);
	return p2;	
}


arbre *ajouter_noeud(FILE *f)
{
    arbre *p = creerArbre();
    uint32_t *a = compter_occurrence(f); // Compter les occurrences
    if (a == NULL) 
    {
        fprintf(stderr, "Erreur d'allocation pour compter les occurrences\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < 256; i++)
    {
        if (a[i] != 0)
        {
            noeud *n = creerNoeud(a[i], (char)i, true); // Créer un nœud
            push(p, n); // Ajouter le nœud à la pile
        }        
    }

    free(a);// Libérer la mémoire du tableau d'occurrences
    return trier_pile(p); // Retourner la pile triée
}



arbre *construire_arbre(arbre *arb) //va prendre la pile trier en paramètre
{
	 while (arb->tete != NULL && arb->tete->next != NULL)

	 { 
		noeud *n1 = pop(arb);// 1er élément de la pile trié
		noeud *n2 = pop(arb);// 2nd élément de la pile trié
		
		noeud *parent= creerNoeud((n1->poids + n2->poids), '\0', false);
    	parent->gauche = n1; //attribut descendant gauche 
    	parent->droite = n2; //attribut descendant droit
		push(arb, parent); //remettre les deux noeuds fusionné dans la pile
		arb = trier_pile(arb); //trier la pile 
			
	}
	return arb;
}


void parcourir_noeud(noeud *n, uint32_t code_bis, uint32_t profondeur)
{
    if (n == NULL) {
        printf(" ERREUR : nœud NULL, retour\n");
        return;
    }

    if (n->est_feuille)
    {
        table_huffman[(unsigned char)n->symbole].code = code_bis; // stocke le code
        table_huffman[(unsigned char)n->symbole].taille = profondeur; // stocke la longueur du code 	
        return;
    }
    if (n->gauche != NULL)
    {
        parcourir_noeud(n->gauche, code_bis << 1, profondeur + 1); //code << 1 fait un décalage à gauche et rajoute 0
    }
    
    if (n->droite != NULL)
    {
        parcourir_noeud(n->droite, (code_bis <<1)|1, profondeur + 1); // (code_bis <<1)|1 fait un decalage à gauche et rajoute 1
    }
}

void liberer_arbre(noeud *racine) {
    if (racine == NULL)
    {
    	return;
    }
    liberer_arbre(racine->gauche);
    liberer_arbre(racine->droite);
    free(racine);
}

void ajouter_bit(buffer_binaire *buf, uint8_t bit, FILE *out)
{
    buf->buffer = (buf->buffer << 1) | (bit & 1); //permet de decaler tous les bits à gauche pour faire de la place pour le nouveau bit et ensuite (bit & 1) prend uniquement le dernier bit de la variable bit et ajoute le nv bit au buffer
    buf->nb_bit++;

    if (buf->nb_bit == 8) // Si 8 bits remplis, écrire dans le fichier
    {
        fwrite(&(buf->buffer), sizeof(uint8_t), 1, out); //ecrire dans le fichier bit par bit 
        //réinitialisation
        buf->buffer = 0; 
        buf->nb_bit = 0;
    }
}

void vider_buffer(buffer_binaire *buf, FILE *out)
{
    if (buf->nb_bit > 0) //il reste de la place dans le buffer 
    {
        buf->buffer <<= (8 - buf->nb_bit); // On décale les bits vers la gauche pour remplir les cases vides avec des 0 jusqu’à avoir 8 bits.
        fwrite(&(buf->buffer), sizeof(uint8_t), 1, out);
        buf->buffer = 0;
        buf->nb_bit = 0;
    }
}


void ecrire_entete(FILE *out, uint32_t total_bits) {
    uint8_t nb_symboles = 0;

    // Compter le nombre de symboles encodés
    for (uint16_t i = 0; i < 256; i++) {
        if (table_huffman[i].taille > 0) {
            nb_symboles++;
        }
    }
    // Écrire le nombre de symboles uniques
    fwrite(&nb_symboles, sizeof(uint8_t), 1, out);

    // Écrire chaque symbole, sa taille et son code Huffman
    for (uint32_t i = 0; i < 256; i++) 
    {
        if (table_huffman[i].taille > 0) 
        {
            fwrite(&i, sizeof(uint8_t), 1, out);// Symbole
            fwrite(&table_huffman[i].taille, sizeof(uint8_t), 1, out); // Longueur du code
            fwrite(&table_huffman[i].code, sizeof(uint32_t), 1, out);  // Code Huffman
        }
    }
    // pour savoir combien de bits utiles il faut lire en décompression
    fwrite(&total_bits, sizeof(uint32_t), 1, out);
}

void encoder(FILE *in, FILE *out) 
{
    buffer_binaire buf = {0, 0};
    uint32_t total_bits = 0;  // Nombre total de bits compressés

    //calculer total_bits
    char c;
    uint32_t e = fscanf(in, "%c", &c);
    while (e != EOF) 
    {
        total_bits = total_bits + table_huffman[(unsigned char)c].taille;
        e = fscanf(in, "%c", &c);
    }
    rewind(in); // Revenir au début du fichier pour l'encodage

    // Écrire entête avec total_bits
    ecrire_entete(out, total_bits);

    // Écrire les données compressées
    e = fscanf(in, "%c", &c);
    while (e != EOF) 
    {
        uint32_t code = table_huffman[(unsigned char)c].code;
        uint8_t longueur = table_huffman[(unsigned char)c].taille;

        for (int8_t i = (int8_t)longueur - 1; i >= 0; i--) 
        {
            ajouter_bit(&buf, (code >> i) & 1, out);
        }

        e = fscanf(in, "%c", &c);
    }

    vider_buffer(&buf, out);
}


