#define _POSIX_C_SOURCE  200112L

#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <IOop.h>
#include <Params.h>

#define OPERATION 0
#define SECONDPARAMETER 1
#define LENGTHIFSTORE 2

int getPars(pars* param, long fd,bufpars* outOfHeader){
	if (param == NULL) return -1;
	if (outOfHeader == NULL) return -1;
	
	char bufNum[32];//buffer dove memorizzo il numero letto in caso di operazione di store
	int n_char = 0;	//numero di carateri letti
	int numpar = 0;	//indica quale elemento di param modificare 
	int index = 0;	//indica l'indice della stringa dove scrivere il carattere
	int j;			//indice ciclo for
	int found = 0;	//indica se 1 che il carattere newline è stato trovato
	
	//azzero gli elementi di param
	memset((*param).op_name, '\0', 16);
	memset((*param).name, '\0', BUFSIZE);
	memset(bufNum, '\0', 32);
	memset((*outOfHeader).data, '\0', BUFSIZE);
	(*param).len = 0;
	
	
	//leggo un carattere alla volta e a seconda del valore di numpar scrivo in un parametro di param
	while ((n_char = readst(fd, (*outOfHeader).data, BUFSIZE)) >= 0){
	
		if (n_char == 0){
			return -1;
		}
		
		for (j = 0; j < n_char; j++){
			
			if ((*outOfHeader).data[j] == '\0')
				continue;
		
			if ((*outOfHeader).data[j] == '\n'){
				found = 1;
				(*outOfHeader).start = j+1;
				(*outOfHeader).end = n_char;
				break;
			}
		
			if ((*outOfHeader).data[j] == ' '){
				numpar++;
				index = 0;
				if (numpar > 3)	//se il numero di parametri è troppo alto
					return -2;
				continue;
			}
	
			switch(numpar){
				case OPERATION:
					(*param).op_name[index] = (*outOfHeader).data[j];
					index++;
					if (index >= 16)
						return -5;
					break;
				case SECONDPARAMETER:
					(*param).name[index] = (*outOfHeader).data[j];
					index++;
					if (index >= BUFSIZE)
						return -6;
					break;
				case LENGTHIFSTORE:
					bufNum[index] = (*outOfHeader).data[j];
					index++;
					if (index >= BUFSIZE)
						return -7;
					break;
			}
		
		}
		
		if (found == 1)
			break;
	}
	
	if (n_char < 0)
		return -3;
	
	if (numpar == 3){
		if (((*param).len = strtol(bufNum, (char**)NULL, 10)) == 0)
			return -4;
	}
	
	//printf("pars: %s %s\n",param->op_name,param->name);
	
	return 0;
}
