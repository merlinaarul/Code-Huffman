#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

typedef struct noeud
{
	struct noeud *droite;
	struct noeud *gauche;
	struct noeud *next;
	uint32_t poids;
	char symbole;
	_Bool est_feuille;
}noeud;

typedef struct
{
	uint32_t code;
	uint8_t taille;
}code_huff;

typedef struct {
    uint8_t buffer;  // Contient les bits avant écriture
    int nb_bit;   // Nombre de bits stockés dans le buffer
}buffer_binaire;
	
typedef struct
{
	noeud *tete;
}arbre;

code_huff table_huffman[256]; 


void lire_entete(FILE *in, uint32_t *total_bits);
arbre *reconstruire_arbre();
void decoder(FILE *in, FILE *out, noeud *n, uint32_t total_bits);
void liberer_arbre(noeud *racine);
arbre *creerArbre();
noeud *creerNoeud(uint32_t poids, char symbole, _Bool est_feuille);

int main(int argc, char *argv[])
{
	FILE *in = fopen(argv[1], "rb");
	if (in == NULL)
	{
    	perror("Erreur lors de l'ouverture du fichier");
    	return EXIT_FAILURE;
    }
    

    uint32_t total_bits;
    lire_entete(in, &total_bits);
    arbre *a = reconstruire_arbre();
    FILE *out = fopen(argv[2], "wb");
    if (out == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        //fclose(in);
   		liberer_arbre(a->tete);
   		free(a);
        return EXIT_FAILURE;
    }
    decoder(in, out, a->tete, total_bits);
    fclose(in);
    fclose(out);
    liberer_arbre(a->tete);
    free(a);
    return EXIT_SUCCESS;
}
    

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

void lire_entete(FILE *in, uint32_t *total_bits) {
    uint8_t nb_symboles;

    // Lire le nombre de symboles uniques
    fread(&nb_symboles, sizeof(uint8_t), 1, in);

    // Réinitialiser la table
    for (uint16_t i = 0; i < 256; i++) {
        table_huffman[i].code = 0;
        table_huffman[i].taille = 0;
    }

    // Lire chaque symbole, sa taille et son code
    for (uint8_t i = 0; i < nb_symboles; i++) {
        uint8_t symbole;
        uint8_t taille;
        uint32_t code;

        fread(&symbole, sizeof(uint8_t), 1, in);
        fread(&taille, sizeof(uint8_t), 1, in);
        fread(&code, sizeof(uint32_t), 1, in);

        table_huffman[symbole].taille = taille;
        table_huffman[symbole].code = code;
    }

    // Lire le nombre total de bits utiles
    fread(total_bits, sizeof(uint32_t), 1, in);
}

arbre *reconstruire_arbre()
{
	arbre *a = creerArbre();
	a->tete = creerNoeud(0, '\0', false);
	for (uint32_t i = 0; i < 256; i++) 
    {
     	if (table_huffman[i].taille > 0) 
        {
        	noeud *temp = a->tete;
        	uint32_t code = table_huffman[i].code;
        	uint8_t longueur = table_huffman[i].taille;
        	
        	for(int8_t j = longueur-1; j>=0; j--)
        	{
        		uint8_t bit = (code >> j) & 1;
        		
        		if(bit == 0)
        		{
        			if(temp->gauche == NULL)
        			{
        				temp->gauche = creerNoeud(0, '\0', false);
        			}
        			temp = temp->gauche;
        		}
        		else
        		{
        			if(temp->droite == NULL)
        			{
        				temp->droite = creerNoeud(0, '\0', false);
        			}
        			temp = temp->droite;
        		}
        	}
        	temp->symbole = (unsigned char)i;
        	temp->est_feuille = true;
        }
	}
	
	return a;
}

void decoder(FILE *in, FILE *out, noeud *n, uint32_t total_bit)
{
	
	buffer_binaire buf = {0, 0};
	noeud *temp = n;
	for(uint32_t i = 0; i < total_bit; i++)
	{
		if(buf.nb_bit == 0)
		{
			if (fread(&(buf.buffer), sizeof(uint8_t), 1, in) != 1) 
			{
		        return; //Indique la fin du fichier
		    }
		    buf.nb_bit = 8; //Remettre 8 bits à disposition
        }
		uint8_t bit = (buf.buffer >> 7) & 1;
		buf.buffer <<= 1;
		buf.nb_bit--;
		if(bit == 0 && (!temp->est_feuille))
		{
			temp = temp->gauche;
		}
		else if(bit == 1 && (!temp->est_feuille))
		{
			temp = temp->droite;
		}
	
		if(temp->est_feuille)
		{
			fwrite(&(temp->symbole), sizeof(char), 1, out);
			temp = n;
		}
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


