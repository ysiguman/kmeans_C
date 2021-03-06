/* inclusion des librairies */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>


//#####################################
//### K ==> NOMBRE DE NOYEAUX
//### N ==> NOMBRE D'ITERATION
//#####################################
#define K 7
#define N 1


#pragma pack(1)
//#####################################
//### STRUCTURE DE L ENTETE DE L IMAGE
//#####################################
struct headerImg
{
	int size;
	int width;
	int height;
	short plans;
	short depth;
	int compr;
	int sizeTotal;
	int hRes;
	int vRes;
	int nbColor;
	int nbColorImp;
};


//#####################################
//### STRUCTURE DE L ENTETE DU FICHIER
//#####################################
struct headerFile
{
	char sign[2];
	int size;
	int reserved;
	int offset;

	struct headerImg img;
};

typedef struct
{
	unsigned char b;
	unsigned char g;
	unsigned char r;

} color;


//#####################################
//### STRUCTURE CLUSTERS
//#####################################
typedef struct 
{
	unsigned int x;
	unsigned int y;

	unsigned int totalX;
	unsigned int totalY;
	unsigned int nbPixels;

	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned int totalR;
	unsigned int totalG;
	unsigned int totalB;

} clusters;


//#####################################
//### INITIALISATION DES FONCTIONS
//#####################################
void kmeans(color **tab, int width, int height, struct headerFile header);
void iterate(clusters cluster[K], color **tab, int width, int height);
int findNearestCluster(clusters cluster[K], color **tab, int x, int y);
int dist(int xa, int xb, int ya, int yb);
void drawCluster(clusters cluster[K], color **tab, int width, int height);
int rgbDiff(clusters cluster, color tab);
void create(struct headerFile header, color **tabColor, int nb);
void drawTest(clusters cluster[K], color **tab, int width, int height, struct headerFile header, int increment);


//#####################################
//### FONCTION MAIN
//#####################################
int main(int argc, char const *argv[])
{
	//#####################################
	//### DECLARATION DES VARIABLES
	//#####################################
	FILE *fichier = NULL;
	int i = 0, j = 0;
	color **tabColor = NULL;

	fichier = fopen("lenaColor.bmp", "rb");

	struct headerFile header;


	//#####################################
	//### CREATION DU TABLEAU & LECTURE DU FICHIER
	//#####################################
	fread(&header, sizeof(header), 1, fichier);

	tabColor = ( color ** ) malloc( header.img.width * ( sizeof(color*) ));
	for (i = 0; i < header.img.height; ++i)
	{
		tabColor[i] = ( color * ) malloc( header.img.width * ( sizeof(color) ));
	}

	for (i = header.img.height - 1; i >= 0; --i)
	{
		for (j = 0; j < header.img.width; ++j)
		{
			fread(&tabColor[i][j], sizeof( tabColor[i][j] ), 1, fichier);
		}
	}


	//#####################################
	//### LANCEMENT DE LA METHODE KMEANS & FERMETURE
	//#####################################
	kmeans(tabColor, header.img.width, header.img.height, header);

	create(header, tabColor, 0);
	fclose(fichier);
	
	return 0;

}


void kmeans(color **tab, int width, int height, struct headerFile header)
{
	//#####################################
	//### DECLARATION DES VARIABLES
	//#####################################
	int i = 0, j = 0, x = 0, y = 0;
	clusters cluster[ K ];


	//----------------------------------
	// - Definition des noyeaux
	//---------------------------------- 
	srand(time(NULL));
	for (i = 0; i < K; ++i)
	{
		// On selectionne un pixel au hazard
		cluster[i].x = rand()%width;
		cluster[i].y = rand()%height;

		// On "ajoute" au total le 1er pixel
		cluster[i].totalX = cluster[i].x;
		cluster[i].totalY = cluster[i].y;

		// On récupère les couleurs de ce pixel
		cluster[i].b = tab[cluster[i].x][cluster[i].y].b;
		cluster[i].g = tab[cluster[i].x][cluster[i].y].g;
		cluster[i].r = tab[cluster[i].x][cluster[i].y].r;

		// On "ajoute" au total des couleurs les couleurs du pixel
		cluster[i].totalB = cluster[i].b;
		cluster[i].totalG = cluster[i].g;
		cluster[i].totalR = cluster[i].r;

		// On établi le nombre de pixel
		cluster[i].nbPixels = 1;
	}


	//----------------------------------
	// - Itération x N (voir var globales)
	//----------------------------------
	for (i = 0; i < N; ++i)
	{
		iterate(cluster, tab, width, height);

		// drawTest ::> fonction qui colorie une image "copie" et qui l'enregistre
		//drawTest(cluster, tab, width, height, header, i+1);
	}


	//----------------------------------
	// - On desinne les clusters
	//----------------------------------
	drawCluster(cluster, tab, width, height);

	//----------------------------------
	// - Paramètres d'entrée
	//----------------------------------
	printf("- Paramètre d'entree: - \n");
	printf("Nombre de noyeaux: %d \n"
		"Nombre d'itération: %d\n\n", K, N);

	//----------------------------------
	// - On affiche les données sur les diff cluster
	//----------------------------------
	for (i = 0; i < K; ++i)
	{
		printf("- Cluster n°%d -\n", i + 1);
		printf("Couleur: rgb(%d, %d, %d)\n", cluster[i].r, cluster[i].g, cluster[i].b);
		printf("Nombre de pixels: %d\n\n", cluster[i].nbPixels);
	}
}


void iterate(clusters cluster[K], color **tab, int width, int height)
{
	//#####################################
	//### DECLARATION DES VARIABLES
	//#####################################
	int i = 0, j = 0, index = 0;


	//---------------------------------- 
	// - Affectation des pixels a un cluster
	//---------------------------------- 
	for (i = 0; i < width; ++i)
	{
		for (j = 0; j < height; ++j)
		{
			// findNearestCluster ::> Rếcupère le cluster le plus proche
			index = findNearestCluster(cluster, tab, i, j);
			
			// On incrémente le nb de pixel du cluster
			cluster[index].nbPixels++;

			// On ajoute les coordonées du nouveau pixels aux coord totales
			cluster[index].totalX += i;
			cluster[index].totalY += j;

			// On ajoute les couleurs du nv pixel aux couleurs totales
			cluster[index].totalB += tab[i][j].b;
			cluster[index].totalG += tab[i][j].g;
			cluster[index].totalR += tab[i][j].r;
		}
	}


	//---------------------------------- 
	// - Re-evaluation des noyeaux
	//---------------------------------- 
	for (i = 0; i < K; ++i)
	{
		// On rècupere les nouvelles coordonées (inutiles si on gere par couleur)
		cluster[i].x = cluster[i].totalX / cluster[i].nbPixels;
		cluster[i].y = cluster[i].totalY / cluster[i].nbPixels;

		// On rèupère la nouvelle couleur ==> moyenne des couleures totales
		cluster[i].b = cluster[i].totalB / cluster[i].nbPixels;
		cluster[i].g = cluster[i].totalG / cluster[i].nbPixels;
		cluster[i].r = cluster[i].totalR / cluster[i].nbPixels;

		// On réinitialise le cluster
		cluster[i].nbPixels = 1;
		cluster[i].totalY = cluster[i].y;
		cluster[i].totalX = cluster[i].x;

		cluster[i].totalB = cluster[i].b;
		cluster[i].totalG = cluster[i].g;
		cluster[i].totalR = cluster[i].r;

		// On récupère les couleurs du nouveau noyeau (inutile si on gere par couleur)
		// cluster[i].b = tab[cluster[i].x][cluster[i].y].b;
		// cluster[i].g = tab[cluster[i].x][cluster[i].y].g;
		// cluster[i].r = tab[cluster[i].x][cluster[i].y].r;
	}

}


int findNearestCluster(clusters cluster[K], color **tab, int x, int y)
{
//#####################################
//### DECLARATION DES VARIABLES
//#####################################
	int i = 0, j = 0;

	for (i = 1; i < K; ++i)
	{
		// Si on gere par distance par rappor au noyeau (diagramme de Voronoi)
		// if (dist(cluster[i].x, cluster[i].y, x, y) < dist(cluster[j].x, cluster[j].y, x, y))
		// {
		// 	j = i;
		// }

		// Si on gere par couleur ==> rgbDiff ::> Différence entre 2 couleurs
		if (rgbDiff(cluster[i], tab[x][y]) < rgbDiff(cluster[j], tab[x][y]) )
		{
			j = i;
		}
	}
	return j;
}


int dist(int xa, int ya, int xb, int yb)
{
	//#####################################
	//### FORMULE DISTANCE ENTRE 2 POINTS:
	//### AB = racine((xB - xA)**2 + (yB - yA)**2)
	//#####################################
	int x = xb - xa;
	int y = yb - ya;

	return sqrt(pow(x,2) + pow(y,2));
}


int rgbDiff(clusters cluster, color tab)
{
	//#####################################
	//### RECUPERE LES DONNEES DE COULEUR
	//### RENVOIE LA DIFFERENCE
	//#####################################
	int r = cluster.r - tab.r;
	int g = cluster.g - tab.g;
	int b = cluster.b - tab.b;

	return sqrt(pow(r,2) + pow(g,2) + pow(b,2));
}


void drawCluster(clusters cluster[K], color **tab, int width, int height)
{
	//#####################################
	//### DECLARATION DES VARIABLES
	//#####################################
	int i = 0, j = 0, index = 0;


	//---------------------------------- 
	// - On dessine les cluster sur l'image
	//---------------------------------- 
	for (i = 0; i < width; ++i)
	{
		for (j = 0; j < height; ++j)
		{
			// On recuperer le cluster le plus proche
			index = findNearestCluster(cluster, tab, i, j);
			
			tab[i][j].r = cluster[index].r;
			tab[i][j].b = cluster[index].b;
			tab[i][j].g = cluster[index].g;

			cluster[index].nbPixels++;
		}
	}
}

void create(struct headerFile header, color **tabColor, int nb)
{
	//#####################################
	//### DECLARATION DES VARIABLES
	//#####################################
	int i = 0, j = 0;
	
	// Préparation du fichier de sortie
	FILE *fichierOut = NULL;
	char filename[20];
	// On nomme le fichier
	sprintf(filename, "%s%d%s","Out/", nb, "-lenaOut.bmp");
	fichierOut = fopen(filename, "wb");


	//---------------------------------- 
	// - On ecrit le fichier
	//---------------------------------- 
	fwrite(&header, sizeof(header), 1, fichierOut);

	for (i = header.img.height - 1; i >= 0; --i)
	{
		for (j = 0; j < header.img.width; ++j)
		{
			fwrite(&tabColor[i][j], sizeof( tabColor[i][j] ), 1, fichierOut);
		}
	}

	fclose(fichierOut);
}


void drawTest(clusters cluster[K], color **tab, int width, int height, struct headerFile header, int increment)
{
	//#####################################
	//### DECLARATION DES VARIABLES
	//#####################################
	color **tabCopie;
	int i = 0, j = 0;


	//---------------------------------- 
	// - Creation du tableau
	//---------------------------------- 
	tabCopie = ( color ** ) malloc( width * ( sizeof(color*) ));

	for (i = 0; i < height; ++i)
	{
		tabCopie[i] = ( color * ) malloc( width * ( sizeof(color) ));
	}


	//---------------------------------- 
	// - On copie le tableau
	//---------------------------------- 
	for (i = 0; i < height; ++i)
	{
		for (j = 0; j < width; ++j)
		{
			tabCopie[i][j].r = tab[i][j].r;
			tabCopie[i][j].g = tab[i][j].g;
			tabCopie[i][j].b = tab[i][j].b;
		}
	}


	//---------------------------------- 
	// - On dessine les clusters et on écrit le nouveau tableau dans un fichier
	//---------------------------------- 
	drawCluster(cluster, tabCopie, width, height);
	create(header, tabCopie, increment);
}