#define _POSIX_C_SOURCE  200112L

#include <ObjStoreOperation.h>
#include <Params.h>
#include <IOop.h>

#include <sys/types.h>         
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#define MAXTENTATIVI 3

#define ERROR(cl,rv,s) \
	if (s == 1) \
		fprintf(stderr,"%s",cl); \
	else \
		fprintf(stderr,"KO %s\n",cl); \
	return rv;

#define ERRORERRNO(rv) \
	char buf[128]; \
	strerror_r(errno,buf,128);\
	fprintf(stderr,"KO %s\n",buf); \
	return rv;

#define NOEXITERRNO \
	char buf[128]; \
	strerror_r(errno,buf,128);\
	fprintf(stderr,"KO %s\n",buf); \

#define UNIX_PATH_MAX 108
#define SOCKNAME "./objstore.sock"

#define true 1
#define false 0

int client_fd = -1;
sigset_t objStorOperationOldMask;
/*
REQUIRES: name != NULL
ERRORS: name = NULL, impossibile collegarsi al server oppure collegamento già avvenuto
EFFECTS: in caso di successo crea una connessione con il server
RETURNS: true(1) in caso di successo, false(0) altrimenti
*/
int os_connect(char *name){	
	if (!name) {ERROR("Invalid Argument (name)",false,0)}
	if (strlen(name) <= 0) {ERROR("Invalid Argument (name)",false,0)}
	if (client_fd != -1) {ERROR("Invalid Argument (already connected)",false,0)}
	
	struct sockaddr_un sa;
	char buffer[BUFSIZE];
	int error;
	int tries = 0;
	sigset_t pset;
	
	//maschero il segnale di SIGPIPE
	if (sigemptyset(&pset) == -1)
		{ERRORERRNO(false)}
	if(sigaddset(&pset,SIGPIPE) == -1)
		{ERRORERRNO(false)}
	if (pthread_sigmask(SIG_SETMASK,&pset,&objStorOperationOldMask) != 0)
		{ERRORERRNO(false)}
	
	strncpy(sa.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);
	
	sa.sun_family = AF_UNIX;
	
	client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client_fd == -1){
		{ERRORERRNO(false)}
	}
	
	while(connect(client_fd, (struct sockaddr*) &sa, sizeof(sa)) == -1 && tries < MAXTENTATIVI){
		if (errno == ENOENT){	//file del socket non esistente
			tries++;
			sleep(1);
		}else{
			ERRORERRNO(false)
		}	
	}
	
	if (tries >= MAXTENTATIVI){
		ERROR("Unable To Connect To Server",false,0);
	}
	
	memset(buffer, '\0', BUFSIZE);
	
	error = snprintf(buffer,BUFSIZE,"REGISTER %s \n", name);
	if (error < 0) {
		ERRORERRNO(false)
	}
	
	if (writen(client_fd,buffer,strlen(buffer)) == -1){
		ERRORERRNO(false)
	}
	
	memset(buffer, '\0', BUFSIZE);
	
	if (readst(client_fd,buffer,BUFSIZE) == -1){
		ERRORERRNO(false)
	}
	
	int ris = strcmp(buffer,"OK \n");
	
	if (ris != 0){
		ERROR(buffer,false,1)
	}
	
	return true;
}

int os_store(char *name, void *block, size_t len){
	if (client_fd == -1) {ERROR("Invalid Argument (not connected)",false,0)}	//client non connesso
	
	if (name == NULL) {ERROR("Invalid Argument (name)",false,0)}
	if (block == NULL) {ERROR("Invalid Argument (block)",false,0)}
	if (len <= 0) {ERROR("Invalid Argument (len)",false,0)}
	
	char header[BUFSIZE];
	int error;
	
	memset(header,'\0',BUFSIZE);
	
	error = snprintf(header,BUFSIZE,"STORE %s %ld \n", name, (long) len);
	if (error < 0) {
		ERRORERRNO(false)
	}
	
	if (writen(client_fd,header,strlen(header)) == -1){
		NOEXITERRNO
		return false;
	}
	
	if (writen(client_fd,block,len) == -1){
		NOEXITERRNO
		return false;
	}
	
	memset(header, '\0', BUFSIZE);
	
	if (readst(client_fd,header,BUFSIZE) == -1){
		ERRORERRNO(false)
	}
	
	int ris = strcmp(header,"OK \n");
	
	if (ris != 0){
		ERROR(header,false,1)
	}
	
	return true;
}

void *os_retrieve(char *name){
	if (client_fd == -1) {ERROR("Invalid Argument (not connected)",NULL,0)}	//client non connesso
	
	if (name == NULL) {ERROR("Invalid Argument (name)",NULL,0)}
	
	char header[BUFSIZE];
	char* fileBuffer = NULL;
	int error;
	long length;
	int alreadyRead;
	pars param;
	bufpars outOfHeader;
	
	memset(header,'\0',BUFSIZE);

	//invio richiesta di RETRIEVE
	error = snprintf(header,BUFSIZE,"RETRIEVE %s \n", name);
	if (error < 0) {
		ERRORERRNO(NULL)
	}
	
	if (writen(client_fd,header,strlen(header)) == -1){
		ERRORERRNO(NULL)
	}
	
	
	//Ricevo la risposta e verifico che l'operazione sia andata a buon fine
	if (getPars(&param,client_fd,&outOfHeader) == -1){
		ERROR("Bad Parameter Format",NULL,0);
	}
	
	if(strncmp(param.op_name,"KO",2) == 0){
		ERROR(param.name,NULL,0);
	}
	
	length = strtol(param.name, (char**)NULL, 10);
	if (length == 0)
		{ERRORERRNO(NULL)}
	
	//memorizzo il file nel buffer
	fileBuffer = (char*) malloc(sizeof(char)*(length+1));
	
	if (fileBuffer == NULL){
		ERROR("Invalid Pointer",NULL,0);
	}
	
	memset(fileBuffer,'\0',(length+1));
	
	alreadyRead = outOfHeader.end - outOfHeader.start;
	
	//copio i dati già letti nel buffer
	strcpy(fileBuffer,outOfHeader.data + outOfHeader.start);
	
	
	//leggo i nuovi dati
	if (readn(client_fd,fileBuffer + alreadyRead,length - alreadyRead) == -1){
		NOEXITERRNO
		free(fileBuffer);
		return NULL;
	}
	
	return (void*) fileBuffer;
}

int os_delete(char *name){
	if (client_fd == -1) {ERROR("Invalid Argument (not connected)",false,0)}	//client non connesso
	
	if (name == NULL) {ERROR("Invalid Argument (name)",false,0)}

	char buffer[BUFSIZE];
	int error;
	
	memset(buffer, '\0', BUFSIZE);
	
	error = snprintf(buffer,BUFSIZE,"DELETE %s \n", name);
	if (error < 0) {
		ERRORERRNO(false)
	}
	
	if (writen(client_fd,buffer,strlen(buffer)) == -1){
		ERRORERRNO(false)
	}
	
	memset(buffer, '\0', BUFSIZE);
	
	if (readst(client_fd,buffer,BUFSIZE) == -1){
		ERRORERRNO(false)
	}
	
	int ris = strcmp(buffer,"OK \n");
	
	if (ris != 0){
		ERROR(buffer,false,1)
	}
	
	return true;
}

int os_disconnect(){
	if (client_fd == -1) {ERROR("Invalid Argument (not connected)",false,0)}	//client non connesso
	
	char buffer[BUFSIZE];
	int error;
	
	error = snprintf(buffer,BUFSIZE,"LEAVE \n");
	if (error < 0) {
		ERRORERRNO(false)
	}
	
	if (writen(client_fd,buffer,strlen(buffer)) == -1){
		ERRORERRNO(false)
	}
	
	memset(buffer, '\0', BUFSIZE);
	
	if (readst(client_fd,buffer,BUFSIZE) == -1){
		ERRORERRNO(false)
	}
	
	int ris = strcmp(buffer,"OK \n");
	
	if (ris != 0){
		ERROR(buffer,false,1);
	}
	
	client_fd = -1;
	
	//rimetto la vecchia maschera
	if (pthread_sigmask(SIG_SETMASK,&objStorOperationOldMask,NULL) != 0)
		{ERRORERRNO(false)}
	
	return true;
} 
