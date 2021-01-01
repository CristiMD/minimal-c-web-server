#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
//Portul pe care se asculta
//#define PORT 80
int *port_adr, PORT;
char entrypoint[256];
char folder_intial[256];
//Tipurile de continut pe care le poate returna serverul
char extensii[7][15] = {"image/jpeg",  "image/png", "text/plain", "text/html", "text/css", "text/javascript"};

//Realizeaza conversia resursei din request in calea necesara deschiderii fisierului pe server tinand cont de folderul initial
char* construiesteCale (char* numeFisier) {
	//Folder initial
    const size_t sLength = strlen(numeFisier);
	const size_t pathLength = strlen(folder_intial);
    const size_t totalLength = sLength + pathLength;
	//Alocare memorie pentru calea finala
    char *const buffer = malloc(totalLength + 1);
	//Copiere cale in sirul final
	strcpy(buffer, folder_intial);
    strcpy(buffer + pathLength, numeFisier);
    return buffer;
}

//Verificare existenta fisier pe server
int existaFisier(char* numeFisier){
	FILE *fisier;
	//Formatare cale fisier
	char *test = construiesteCale(numeFisier);
	fisier = fopen(test, "r");
	//Eliberare memorie cale fisier
	free(test);
	//Testare deschidere fisier, daca fisierul nu exita returnam 0
	if(fisier == NULL){
		return 0;
	} else {
		fclose(fisier);
		return 1;
	}
}

char* citesteFisier(char* numeFisier, long *numBytes, int extensie){

	//Formatare cale fisier
	char *test = construiesteCale(numeFisier);
	char  *buffer;
	FILE *fisier;
	fisier = fopen(test, "r");
	//Eliberare memorie cale fisier
	free(test);
    //Dimensiune fisier
    fseek(fisier, 0L, SEEK_END);
    *numBytes = ftell(fisier);
    //Resetare pozitie la inceputul fisierului
    fseek(fisier, 0L, SEEK_SET);
	
    //Alocare memorie pentru citirea fisierului
    buffer = (char*)calloc(*numBytes, sizeof(char));
		
	//Citire fisier
    fread(buffer, sizeof(char), *numBytes, fisier);
    fclose(fisier);
	return buffer;
}

//Descompunere request in parametri necesari 
char* resursa(char* buffer) {
	char * buf;
	char * store[4];
	int i = 0;
	//store[0] = metoda
	//store[1] = calea
	//store[2] = protocol
	buf = strtok (buffer,"\n");
	store[i] = strtok (buf," ");
	while (store[i] != NULL){
		i++;
	    store[i] = strtok (NULL, " ");
	}
	return store[1];
}


//Stabileste extensia de fisier pentru stabilirea header-ului de continut
int gasesteExtensie(const char* numeFisier) {
	char *copie;
	//Realizeaza o copie dupa numele fisierului pentru a nu il modifica
	copie = (char *) malloc( strlen(numeFisier) + 1 ); 
	strcpy( copie, numeFisier );
	//Separare nume fisier dupa caracterul '.'
	char *separator = strchr(copie, '.');
	*separator = '\0';
	//Verificare tip fisier si returnarea index-ului corespunzator din sir-ul de extensii global
	if (strcmp(separator + 1, "html") == 0){
	  	printf("Fisier HTML\n");
		return 3;
	} else if (strcmp(separator + 1, "jpg") == 0) {
		printf("Fisier jpg\n");
		return 0;
	} else if (strcmp(separator + 1, "jpeg") == 0) {
		printf("Fisier jpeg\n");
		return 0;
	} else if (strcmp(separator + 1, "png") == 0) {
		printf("Fisier png\n");
		return 1;
	} else if (strcmp(separator + 1, "txt") == 0) {
		printf("Fisier txt\n");
		return 2;
	} else if (strcmp(separator + 1, "css") == 0) {
		printf("Fisier css\n");
		return 4;
	} else if (strcmp(separator + 1, "js") == 0) {
		printf("Fisier js\n");
		return 5;
	} else {
		printf("Fisier negasit");
	}
	//Eliberare memorie copie
	free(copie);
}

//Construieste raspunsul de trimis catre client
char * construiesteRaspuns(char* cod, int extensie, long numarBytes, char* continut) {
	char *res = "HTTP/1.1 ";
	//Obtinere tip continut din sirul de extensii global
	char *tipContinut = extensii[extensie];
	char tip[50];
	//Realizarea unui sir potrivit pentru concatenarea in raspunsul final pentru tipul fisierului
	sprintf(tip, "\nContent-Type: %s\n", tipContinut);
	char numbits[50];
	//Realizarea unui sir potrivit pentru concatenarea in raspunsul final pentru dimensiunea continutului
	sprintf(numbits, "Content-Length: %ld\n\n", numarBytes);
	//Calcularea dimensiunii necesare pentru buffer-ul de raspuns
	long newSize = strlen(res) + strlen(cod) + strlen(tip) + strlen(numbits) + strlen(continut) + 1;
	//Alocarea memoriei necesare pentru buffer-ul de raspuns
	char * newBuffer = (char *)malloc(newSize);
	//Copiaza si concateneaza informatiile intr-un singur sir in buffer-ul de raspuns
	strcpy(newBuffer, res);
	strcat(newBuffer, cod);
	strcat(newBuffer, tip);
	strcat(newBuffer, numbits);
	strcat(newBuffer, continut);
	return newBuffer;
}


//Functie ce combina citirea fisierului si realizarea raspunsului in cazul care fisierul solicitat este o imagine
//Folosim aceasta functie pentru lucrul cu citirea fisierelor binare si evitarea alterarii reazultatului citirii
int citesteImagine(int new_socket, char* numeFisier) {
	long  len;
	char* buf = NULL;
	FILE* fp  = NULL;
	char *test = construiesteCale(numeFisier);
	//Deschiderea fisierului in modul de citire binara
	fp = fopen( test, "rb" );
  	//Gasirea dimensiunii fisierului
	if (fseek( fp, 0, SEEK_END ) != 0){
    	fclose( fp );
    	return 0;
    }
	//Inregistrarea dimensiunii fisierului intr-o variabila si resetarea positiei la inceputul fisierului
	len = ftell( fp );
	rewind( fp );

	//Alocare memorie pentru bufferul de continut
	buf = (char*)malloc( len );
  	if (!buf){
    	fclose( fp );
    	return 0;
    }
  	//Citire continut fisier in buffer
 	if (!fread( buf, len, 1, fp )){
    	free( buf );
    	fclose( fp );
    	return 0;
    }

  	fclose( fp );

	//Realizarea header-ului pentru client
	char *res = "HTTP/1.1 ";
	//char *tipContinut = extensii[extensie];
	//char tip[50];
	char *cod = "200 OK";
	char *tip = "\nContent-Type: image/jpeg\n";
	char numbits[50];
	sprintf(numbits, "Content-Length: %ld\n\n", len);
	//Calcularea dimensiunii necesare pentru buffer-ul de raspuns
	long newSize = strlen(res) + strlen(cod) + strlen(tip) + strlen(numbits) + len + 1;
	//Calcularea dimensiunii necesare pentru buffer-ul fara continut
	long toCpy = strlen(res) + strlen(cod) + strlen(tip) + strlen(numbits);
	//Alocarea dimensiunii necesare pentru buffer-ul de raspuns
	char * newBuffer = (char *)malloc(newSize);
	//Copiaza si concateneaza informatiile intr-un singur sir in buffer-ul de raspuns
	strcpy(newBuffer, res);
	strcat(newBuffer, cod);
	strcat(newBuffer, tip);
	strcat(newBuffer, numbits);
	//Copiaza si concateneaza informatiile din imagine in bufferul de raspuns
	//Folosim memcpy pentru a nu altera informatiile in format binar
	memcpy(newBuffer+toCpy, buf, len);
	//Trimiterea raspunsului catre client
 	write(new_socket , newBuffer , newSize);
	//Eliberarea memoriei pentru buffer-ul de raspuns
 	free( buf );
	return 1;
}

void obtineConfigurari(int* port, char* entrypoint, char* folder_intial){
	FILE * fp;
    char * linie = NULL;
    size_t len = 0;
	int index = 0;
	//Deschidere fisier configurari
    fp = fopen("server.config", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
	//Fisierul de configurari trebuie sa aiba doar 3 linii din care sa citim valorile
    while (index < 3) {
		//Citim linia curenta
		getline(&linie, &len, fp);
		//Obtinem valoarea de pe line de dupa semnul =
		char *separator = strchr(linie, '=');
		*separator = '\0';
		//In functie de numarul liniei stabilim variabila citita
		if(index == 0) {
			//Convertim sirul citit in numar pentru utilizare
			*port = atoi(separator+1);
		} else if (index == 1) {
			//Utilizam strtok pentru a scapa de newline-ul de la finalul sirului citit
			strtok(separator+1, "\n");
			memcpy(entrypoint, separator+1, len);
		} else if (index == 2) {
			//Utilizam strtok pentru a scapa de newline-ul de la finalul sirului citit
			strtok(separator+1, "\n");
			memcpy(folder_intial, separator+1, len);
		}
		index++;
    }

    fclose(fp);
    if (linie){
		free(linie);
	}
}

void fisierNegasit(int new_socket, long int* numBytes, long int numarBytes) {
	char *buffer;
	//Citirea informatiilor din fisier
	buffer = citesteFisier("/404.html", numBytes, 3);
	//Obtinerea raspunsului pentru client
	char *rezultatD = construiesteRaspuns("404 Not Found", 3, numarBytes, buffer);
	//Eliberarea memoriei pentru buffer-ul de citire
	free(buffer);
	//Trimiterea raspunsului catre client
	write(new_socket , rezultatD , strlen(rezultatD));
	//Eliberarea memoriei pentru buffer-ul de raspuns
	free(rezultatD);
}

int main(int argc, char const *argv[]){

    int server_fd, new_socket; long valread; char*  test;
	long *numBytes, numarBytes;
	numBytes = &numarBytes;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
	port_adr = &PORT;

	obtineConfigurari(port_adr,entrypoint,folder_intial);
    
	// Creare socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Eroare socket: ");
        exit(EXIT_FAILURE);
    }

	//Setarea parametrilor pentru structura adresei
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    memset(address.sin_zero, '\0', sizeof address.sin_zero);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("Eroare bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0){
        perror("Eroare ascultare");
        exit(EXIT_FAILURE);
    }

    while(1){
        printf("\n----- Astepare conectiune -----\n\n");
		//Verificare stare socket
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
            perror("Eroare acceptare");
            exit(EXIT_FAILURE);
        }
		//Realizare sir pentru primirea request-ului
        char buffer_1[30000] = {0};
		//Citirea request-ului in buffer
        valread = read( new_socket , buffer_1, 30000);
		//Formatarea buffer-ului pentru returnarea caii fisierului solicitat
		char *cale = resursa(buffer_1);
		int initial;
		//Comparam calea pentru a stabili daca afisam un fisier sau entrypoint-ul prestabilit
		initial = strcmp(cale, "/");
		int ext;
		//Daca fisierul exista stabilim extensia lui
		int exista = existaFisier(cale);
		if(!exista || initial == 0){
			ext = 9;
		} else {
			ext = gasesteExtensie(cale);
		}

		//Daca calea este / afisam entrypoint-ul prestabilit
		if(initial == 0){
			char *buffer;
			buffer = citesteFisier(entrypoint, numBytes, ext);
			char *rezultatD = construiesteRaspuns("200 OK", ext, numarBytes, buffer);
			free(buffer);
		    write(new_socket , rezultatD , strlen(rezultatD));
			free(rezultatD);
		    printf("---- Raspuns trimis ----");
		//Daca extensia fisierului este jpeg sau png apelam functia de citire imagine
		} else if (ext == 0 || ext == 1){
			citesteImagine(new_socket, cale);
		//Daca conditiile de mai sus nu sunt indeplinite trebuie sa cautam un fisier separat de pe server
		} else {
			//Verificam existenta fisierului
			int exista = existaFisier(cale);
			if(!exista){
				//Dca fisierul nu exista retunam statusul 404 cu fisierul HTML potrivit
				fisierNegasit(new_socket, numBytes, numarBytes);
			} else {
				char *buffer;
				//Citirea informatiilor din fisier
				buffer = citesteFisier(cale, numBytes, ext);
				//Obtinerea raspunsului pentru client
				char *rezultatD = construiesteRaspuns("200 OK", ext, numarBytes, buffer);
				//Eliberarea memoriei pentru buffer-ul de citire
				free(buffer);
				//Trimiterea raspunsului catre client
				write(new_socket , rezultatD , strlen(rezultatD));
				//Eliberarea memoriei pentru buffer-ul de raspuns
				free(rezultatD);
				printf("---- Raspuns trimis ----");
			}
		}
	//Inchiderea socket-ului
	close(new_socket);
    }
    return 0;
}

