#include <IOop.h>
#include <ObjStoreOperation.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

char* testStore = "ABCD4567890123456789012345678901234567890123456789W1234567890123456789012345678901234567890123456789";

void printReport(char* op, int tot, int suc, int fail){
	printf("%s - totali: %d - successi: %d - fallimenti: %d\n",op, tot, suc, fail);
}

int StoreTest(char* Username){
	
	int tot = 0, suc = 0, fail = 0;
	int len = 100;
	int i, j, z;
	char *str;
	char* name;
	
	for (i=0; i<20; i++){
		tot++;
		str = (char*) malloc(sizeof(char)*(len+1));
		memset(str,'\0',len);
		if (str == NULL)
			return -1;
		strcpy(str,testStore);

		for (j = 0; j < i; j++)
			for (z = 0; z < 53; z++) 
				strcat(str,testStore);
		name = (char*) malloc(sizeof(char)*BUFSIZE);
		if (name == NULL){
			free(str); return -1; 
		}
		memset(name,'\0',BUFSIZE);
		snprintf(name,BUFSIZE,"%s%d",Username,i);
		if (os_store(name,str,len) == 0)
			fail++;
		else
			suc++;
		
		len += 5300;
		free(str);free(name);
	}
	
	printReport("STORE",tot,suc,fail);
	
	return 0;
}

int RetrieveTest(char* Username){
	
	int tot = 0, suc = 0, fail = 0;
	int len = 100;
	int i, j, z;
	char *str;
	char* ris;
	char* name;
	
	for (i=0; i<20; i++){
		tot++;
		str = (char*) malloc(sizeof(char)*(len+1));
		memset(str,'\0',len);
		if (str == NULL)
			return -1;
		strcpy(str,testStore);

		for (j = 0; j < i; j++)
			for (z = 0; z < 53; z++) 
				strcat(str,testStore);
		name = (char*) malloc(sizeof(char)*BUFSIZE);
		if (name == NULL){
			free(str); return -1; 
		}
		memset(name,'\0',BUFSIZE);
		snprintf(name,BUFSIZE,"%s%d",Username,i);
		if ((ris = os_retrieve(name)) == NULL){
			fail++;
		}
		else{
			if (strcmp(ris,str)==0)
				suc++;
			else{
				printf("Retrieve\n");
				fail++;
			}
		}

		len += 5300;
		
		free(str);free(name);free(ris);
	}
	
	printReport("RETRIEVE",tot,suc,fail);
	
	return 0;
}

int DeleteTest(char* Username){
	int i;
	char* name;
	int tot = 0, suc = 0, fail = 0;
	
	for (i=0; i<20; i++){
		name = (char*) malloc(sizeof(char)*BUFSIZE);
		if (name == NULL){
			return -1; 
		}
		tot++;
		memset(name,'\0',BUFSIZE);
		snprintf(name,BUFSIZE,"%s%d",Username,i);
		if (os_delete(name) == 0)
			fail++;
		else {
			suc++;
		}
		
		free(name);
	}
	
	printReport("DELETE",tot,suc,fail);
	
	return 0;
}

int main(int argc, char* argv[]){
	
	if (argc != 3){
		fprintf(stderr,"Errore, uso: %s <nome Client> <{1,2,3}>",argv[0]);
		return -1;
	}
	
	int ris = 0;
	int test_case = -1;
	
	test_case = strtol(argv[2], NULL, 10);
	
	if (test_case < 1 || test_case > 3){
		fprintf(stderr,"Errore, uso: %s <nome Client> <{1,2,3}>",argv[0]);
		return -1;
	}
	
	ris = os_connect(argv[1]);
	if (ris != 1){
		printReport("CONNECT",1,0,1);
		return -1;
	}else{
		printReport("CONNECT",1,1,0);
	}
	
	switch(test_case){
		case 1: 
			StoreTest(argv[1]);
			break;
		case 2:
			RetrieveTest(argv[1]);
			break;
		case 3:
			DeleteTest(argv[1]);
			break;
	}
	
	ris = os_disconnect(argv[1]);
	if (ris != 1){
		printReport("DISCONNECT",1,0,1);
		return -1;
	}else{
		printReport("DISCONNECT",1,1,0);
	}

	return 0;
}
