#define _POSIX_C_SOURCE  200112L
#define _XOPEN_SOURCE 500

#include <Signals.h>
#include <IOop.h>
#include <Params.h>
#include <Data.h>

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ftw.h>

#define SOCKNAME "./objstore.sock"

#define MAXTHREAD 50
#define MAXEXTRA 50
#define SERVERDIRECTORY "./data"

#define SENDMSG(buffer,fd,toDo)\
	if (writen(fd,buffer,strlen(buffer)) == -1){ \
		perror("writen"); \
		break; \
	} \
	toDo
	
	
sigset_t oldSet;
//sigFlags[0] = 1 indica la ricezione di un qualsiasi segnale, sigFlags[1] = 1 indica la ricezione del segnale SIGUSR1
volatile sig_atomic_t sigFlags[2];
//indica un errore nel thread Main = 1. Indica agli altri thread di terminare una volta che hanno terminato di servire il client.
int ErrorServer;	
					//indica un errore nel main thread
int numClient, numThread;				//indica il numero di client, Thread
int extraThreads;						//indica il numero di thread che è possibile creare sopra la soglia MAXTHREAD
unsigned long long objStoreSizeByte;	//indica la grandezza in byte dell'objstore
unsigned long long objStoreSizeFile;	//indica il numero di file contenuti nell'objstore
unsigned long long objStoreSizeDire;	//indica il numero di directory contenute nell'objstore

pthread_mutex_t mutexNumClient = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexNumThread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexNumByte = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexNumFile = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexNumDire = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t condNumThread = PTHREAD_COND_INITIALIZER;

//EFFECT: numCLient = numClient + v;
void updateNumClient(int v){
	pthread_mutex_lock(&mutexNumClient);
	numClient += v;
	pthread_mutex_unlock(&mutexNumClient);
}

//EFFECT: numThread = numThread + v;
void updateNumThread(int v){
	pthread_mutex_lock(&mutexNumThread);
	numThread += v;
	if ( v < 0)
		pthread_cond_signal(&condNumThread);
	pthread_mutex_unlock(&mutexNumThread);
}

void  updateNumByte(size_t v){
	pthread_mutex_lock(&mutexNumByte);
	objStoreSizeByte += v;
	pthread_mutex_unlock(&mutexNumByte);
}

void updateNumFile(int v){
	pthread_mutex_lock(&mutexNumFile);
	objStoreSizeFile += v;
	pthread_mutex_unlock(&mutexNumFile);
}

void  updateNumDire(int v){
	pthread_mutex_lock(&mutexNumDire);
	objStoreSizeDire += v;
	pthread_mutex_unlock(&mutexNumDire);
}

static void handlerSIGUSR1(int signum){
	sigFlags[1] = 1;
}

static void handlerOtherSignals(int signum){
	sigFlags[0] = 1;
}

void cleanup(){
	unlink(SOCKNAME);
}

void printInfoSUGUSR1(){
	pthread_mutex_lock(&mutexNumClient);
	fprintf(stdout,"Data:\nClients Number: %d\n",numClient);
	pthread_mutex_unlock(&mutexNumClient);
			
	pthread_mutex_lock(&mutexNumThread);
	fprintf(stdout,"Threads Number: %d\n",numThread);
	pthread_mutex_unlock(&mutexNumThread);
			
	pthread_mutex_lock(&mutexNumDire);
	fprintf(stdout,"Directories Number: %llu\n",objStoreSizeDire);
	pthread_mutex_unlock(&mutexNumDire);
			
	pthread_mutex_lock(&mutexNumFile);
	fprintf(stdout,"Files Number: %llu\n",objStoreSizeFile);
	pthread_mutex_unlock(&mutexNumFile);
			
	pthread_mutex_lock(&mutexNumByte);
	fprintf(stdout,"Bytes Number: %llu bytes\n",objStoreSizeByte);
	pthread_mutex_unlock(&mutexNumByte);
}

int get_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    if (tflag == FTW_F){
   		objStoreSizeByte += sb->st_size;
   		objStoreSizeFile++;
   	}
   	if (tflag == FTW_D)
   		objStoreSizeDire++;
   	
	return 0; 
}

//ritorna la lunghezza del file. può essere indicato il descrittore del file o il path del file. 
//Nel dettaglio, se nameFile != NULL, viene usato nameFile altrimenti viene usato fdfile
long get_length(int fdfile, char* namefile){
	struct stat info;
	
	if (namefile == NULL){
		if (fstat(fdfile,&info) == -1)
			return -1;
	}else{
		if (stat(namefile,&info) == -1)
			return -1;
	}
	
	return (long) info.st_size;
}

//Scrive length byte da fdclient a fdfile
int WriteFile(int fdclient, int fdfile, long length, bufpars *outOfHeader){
	
	char* buffer = (char*) malloc(sizeof(char) * BUFSIZEWRFILE);	//buffer utilizzato per la memorizzazione del file inviato dal client
	
	if (buffer == NULL){
		perror("mallocWriteFIle");
		return -1;
	}
	
	long rem = length;
	int read = 0;
	
	if (outOfHeader != NULL){
		//scrittura dati già precedentemente letti
		
		if (writen(fdfile,outOfHeader->data + outOfHeader->start,outOfHeader->end - outOfHeader->start) == -1){
				free(buffer);
				perror("writennWriteFile");
				return -1;
		}
		
		rem -= outOfHeader->end - outOfHeader->start;
	}
	
	while(rem > 0){
		memset(buffer,'\0',BUFSIZEWRFILE);
		
		if (rem > BUFSIZEWRFILE){
			if (readn(fdclient, buffer, BUFSIZEWRFILE) <= 0){
				free(buffer);
				perror("readnWriteFile");
				return -1;
			}
			read = BUFSIZEWRFILE;
		}else{
			if (readn(fdclient, buffer, rem) <= 0){
				free(buffer);
				perror("readnWriteFile");
				return -1;
			}
			read = rem;
		}
		rem -= BUFSIZEWRFILE;
		
		if (writen(fdfile,buffer,read) == -1){ /*read-1*/
			free(buffer);
			perror("writennWriteFile");
			return -1;
		}

	}
	
	free(buffer);
	
	return 0;
}

//Scrive tutti i byte contenuti in fdclient memorizzandoli in fdclient
int ReadFile(int fdclient, int fdfile){
	long length;
	char buffer[BUFSIZE];
	
	memset(&buffer,'\0',BUFSIZE);
	
	if ((length = get_length(fdfile,NULL))==-1)
		return -1;
	
	snprintf(buffer,BUFSIZE,"DATA %ld \n", length);
	
	if (writen(fdclient,buffer,strlen(buffer)) == -1){
		perror("writenReadFile");
		return -1;
	}
		
	return WriteFile(fdfile,fdclient,length,NULL);
}

long DeleteFile(int fdFile, char* Path){
	long length;
	
	if ((length = get_length(fdFile,NULL))==-1)
		return -1;
	
	if (unlink(Path) == -1)
		return -1;
		
	return length;
}

//FIX: in caso di continue, implementare funzione che invia messaggio di errore!!

static void* cmdThread(void *arg){
	
	long fdclient = (long) arg;							
	int r = 0;											//r = -1 significa che il client ha chiuso la connessione o ha richiesto la chiusura
	int registrato = 0;									//indica se la registrazione dell'utente è avvenuto con successo o meno
	char buffer[BUFSIZE];								//buffer in cui memorizzo i dati da scrivere
	char pathdir[BUFSIZE+strlen(SERVERDIRECTORY)];		//buffer in cui memorizza il path della directory del client
	char* name;											//nome utente del client
	pars params;										//struct dove vengono memorizzati gli elementi dell'header del messaggio
	bufpars outOfHeader;
	
	memset(&pathdir,'\0',BUFSIZE+strlen(SERVERDIRECTORY));
	
	while(1){
		
		fd_set fds;
		struct timespec tv = {0,1000000};	//timer della select
			
		FD_ZERO(&fds);
		FD_SET(fdclient,&fds);
		
		if (pselect(fdclient+1, &fds, NULL, NULL, &tv,&oldSet) == -1 && errno != EINTR){
			perror("select");
			snprintf(buffer,BUFSIZE,"KO Internal Error \n");
			SENDMSG(buffer,fdclient,break;)
		}
		
		if (sigFlags[0] != 0 || ErrorServer != 0){
			snprintf(buffer,BUFSIZE,"KO Server Is Down \n");
			SENDMSG(buffer,fdclient,break;)
		}
		
		if (FD_ISSET(fdclient,&fds)){
			
			memset(&buffer,'\0',BUFSIZE);
			
			if ((r = getPars(&params,fdclient,&outOfHeader)) < 0){
				if (r != -1){
					snprintf(buffer,BUFSIZE,"KO Bad Parameter Format \n");
					SENDMSG(buffer,fdclient,continue;)
				}else
					break;	//il client ha chiuso la connessione
			}
			
			/*REGISTER*/
			if (strncmp(params.op_name,"REGISTER",16) == 0){
					name = (char*) malloc(sizeof(char)*BUFSIZE);
					if (name == NULL){
						snprintf(buffer,BUFSIZE,"KO Internal Error \n");
						SENDMSG(buffer,fdclient,break;)
					}
					
					strncpy(name,params.name,BUFSIZE);
					
					//inserisco il nome nella tabella hash. Se l'inserimento fallisce allora lo stesso nome è già loggato
					if (Insert(name) == -1){
						snprintf(buffer,BUFSIZE,"KO User Already Registered \n");
						SENDMSG(buffer,fdclient,continue;)
					}
					
					//creazione della directory
					snprintf(pathdir,BUFSIZE+strlen(SERVERDIRECTORY),"%s/%s",SERVERDIRECTORY,name);
					
					if (mkdir(pathdir,0777) == -1){
						if (errno != EEXIST){
							perror("mkdir");
							snprintf(buffer,BUFSIZE,"KO Internal Error \n");
							SENDMSG(buffer,fdclient,break;)
						}
					}else
						updateNumDire(1);
					
					updateNumClient(1);
					registrato = 1;
					
					snprintf(buffer,BUFSIZE,"OK \n");
					
					#if defined(DEBUG)
						fprintf(stderr,"Servant Thread registered for the Client %s\n",name);
					#endif
					
					SENDMSG(buffer,fdclient,continue;)
			}
			
			/* STORE */
			if (strncmp(params.op_name,"STORE",16) == 0){
				
				char pathfile [BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir)];	//path del file creato
				
				memset(&pathfile,'\0',BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir));
				
				snprintf(pathfile,BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir),"%s/%s", pathdir, params.name);
					
				int file = open(pathfile, O_CREAT | O_WRONLY | O_EXCL, 0700);
				
				if (file == -1){
					if (errno != EEXIST){
						perror("open");
						snprintf(buffer,BUFSIZE,"KO Internal Error \n");
						SENDMSG(buffer,fdclient,continue;)
					}
					else{
						long length;
						if ((length = get_length(-1,pathfile)) == -1){
							perror("get_length");
							snprintf(buffer,BUFSIZE,"KO Internal Error \n");
							SENDMSG(buffer,fdclient,break;)
						}
						
						file = open(pathfile, O_CREAT | O_WRONLY | O_TRUNC, 0700);
						if (file == -1){
								perror("open");
								snprintf(buffer,BUFSIZE,"KO Internal Error \n");
								SENDMSG(buffer,fdclient,continue;)
						}
						updateNumFile(-1);
						updateNumByte(-length);
					}
				}
				
				if (WriteFile(fdclient,file,params.len,&outOfHeader) == -1){
					snprintf(buffer,BUFSIZE,"KO Unable To Save File \n");
					unlink(pathfile);
					close(file);
					SENDMSG(buffer,fdclient,continue;)
				}
				
				close(file);
				
				snprintf(buffer,BUFSIZE,"OK \n");
				
				updateNumFile(1);
				updateNumByte(params.len);
				
				#if defined(DEBUG)
				fprintf(stderr,"%s thread has successfully stored the file named %s of length %ld \n",name,params.name,params.len);
				#endif
					
				SENDMSG(buffer,fdclient,continue;)
			}
			
			/* RETRIEVE */
			if (strncmp(params.op_name,"RETRIEVE",16) == 0){
				
				char pathfile [BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir)];	//path del file creato
				
				memset(&pathfile,'\0',BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir));
				
				snprintf(pathfile,BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir),"%s/%s", pathdir, params.name);
					
				int file = open(pathfile, O_RDONLY, 0700);
				
				if (file == -1){
					if (errno == 2){
						snprintf(buffer,BUFSIZE,"KO Requested File Does Not Exist \n");
						SENDMSG(buffer,fdclient,continue;)
						}
						else{
						perror("open");
						snprintf(buffer,BUFSIZE,"KO Internal Error \n");
						SENDMSG(buffer,fdclient,continue;)
						}
				}
				
				if (ReadFile(fdclient,file) == -1){
					snprintf(buffer,BUFSIZE,"KO Unable To Send File \n");
					close(file);
					SENDMSG(buffer,fdclient,continue;)
				}	
				
				close(file);
				
				#if defined(DEBUG)
				fprintf(stderr,"%s thread has successfully retrieved the file named %s\n",name,params.name);
				#endif
				
				continue;
			}
			
			/* DELETE */
			if (strncmp(params.op_name,"DELETE",16) == 0){
				
				char pathfile [BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir)];	//path del file creato
				
				memset(&pathfile,'\0',BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir));
				
				snprintf(pathfile,BUFSIZE +strlen(SERVERDIRECTORY) + strlen(pathdir),"%s/%s", pathdir, params.name);
				
				int file = open(pathfile, O_RDONLY, 0700);
				long length = 0;
				
				if (file == -1){
					if (errno == 2){
						snprintf(buffer,BUFSIZE,"KO Requested File Does Not Exist \n");
						SENDMSG(buffer,fdclient,continue;)
						}
						else{
						perror("open");
						snprintf(buffer,BUFSIZE,"KO Internal Error \n");
						SENDMSG(buffer,fdclient,continue;)
						}
				}
				
				if ((length = DeleteFile(file,pathfile)) == -1){
					perror("DeleteFile");
					snprintf(buffer,BUFSIZE,"KO Internal Error \n");
					SENDMSG(buffer,fdclient,break;)
				}
				
				close(file);
				
				updateNumFile(-1);
				updateNumByte(-length);
				
				snprintf(buffer,BUFSIZE,"OK \n");
				
				#if defined(DEBUG)
				fprintf(stderr,"%s thread has successfully deleted the file named %s\n",name,params.name);
				#endif
				
				SENDMSG(buffer,fdclient,continue;)
			}
			
			/* LEAVE */
			if (strncmp(params.op_name,"LEAVE",16) == 0){
				snprintf(buffer,BUFSIZE,"OK \n");
				SENDMSG(buffer,fdclient,break;)
			}
		}
	}
	
	/*CHIUSURA DELLA CONNESSIONE*/
	
	#if defined(DEBUG)
	fprintf(stderr,"%s thread is finished\n",name);
	#endif
	
	if (registrato){
		Delete(name);
		updateNumClient(-1);
	}
	
	close(fdclient);
	
	updateNumThread(-1);
	
	return (void*) 0;
}

int createThread(long fdclient){
	pthread_t th;
	pthread_attr_t atth;
	int ris = 0;
	
	/*attendo che il numero di thread sia minore di MAXTHREAD*/
	pthread_mutex_lock(&mutexNumThread);
	
	while(numThread >= (MAXTHREAD + extraThreads) ){
		struct timespec tv = {0,200000};
		
		if (sigFlags[0] == 1)
			return -1;
		
		ris = pthread_cond_timedwait(&condNumThread,&mutexNumThread,&tv);
		
		if (ris != 0){
			if (numThread < MAXEXTRA + MAXTHREAD)
				extraThreads++;
		}
	}
		
	numThread++;
	
	#if defined(DEBUG)
	fprintf(stderr, "Creating new Thread, Threads Number:%d\n Extra Threads Number:%d\n",numThread,extraThreads);
	#endif
	
	pthread_mutex_unlock(&mutexNumThread);
	
	
	if (ris == 0 && extraThreads > 0)
		extraThreads--;
	
	if (pthread_attr_init(&atth) != 0){
		fprintf(stderr, "errore pthread_attr_init\n");
		updateNumThread(-1);
		return -1;
	}
	
	if (pthread_attr_setdetachstate(&atth, PTHREAD_CREATE_DETACHED) != 0){
		fprintf(stderr,"errore pthread_attr_setdetachstate\n");
		pthread_attr_destroy(&atth);
		updateNumThread(-1);
		return -1;
	}
	
	if (pthread_create(&th,&atth,&cmdThread, (void*) fdclient) != 0){
		fprintf(stderr,"errore pthread_create\n");
		pthread_attr_destroy(&atth);
		updateNumThread(-1);
		return -1;
	}
	
	return 0;
}

int main(int argc, char *argv[]){
	
	cleanup();
	atexit(cleanup);
	
	/*GESTIONE SEGNALI*/
	
	//Maschero tutti i segnali
	sigset_t pset;
	
	if (sigfillset(&pset) == -1)
		return -1;
	if (setMask(pset, &oldSet) == -1)
		return -1;
	
	//istallo i gestori
	if (setHandlerSignals(SIG_IGN, 1, SIGPIPE) == -1)		//ignoro sigpipe
		return -1;
	
	if (setHandlerSignals(handlerSIGUSR1,1,SIGUSR1) == -1)	//in caso di segnale SIGUSR1 uso l'handler handlerSIGUSR1
		return -1;
	
	//in tutti gli altri casi uso l'altro gestore
	if (setHandlerSignals(handlerOtherSignals,6,SIGINT,SIGTERM,SIGQUIT,SIGTSTP,SIGALRM,SIGSEGV) == -1)
		return -1;
	
	//inizializzo i flag
	sigFlags[0] = 0;
	sigFlags[1] = 0;
	
	#if defined(DEBUG)
	fprintf(stderr,"Signal Handler Array and Signal Mask successfully modified\n");
	#endif
	
	/*****************/
	
	/*CREAZIONE SOCKET*/
	int listenfd, fdclient;
	struct sockaddr_un sa;
	
	memset(&sa,'0', sizeof(sa));
	
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, SOCKNAME, strlen(SOCKNAME)+1);
	
	if ((listenfd = socket(AF_UNIX,SOCK_STREAM, 0)) == -1){
		perror("socket");
		return -1;
	}
	
	if (bind(listenfd, (struct sockaddr*) &sa, sizeof(sa)) == -1){
		perror("bind");
		return -1;
	}
	
	if (listen(listenfd,SOMAXCONN) == -1){
		perror("listen");
		return -1;
	}
	
	#if defined(DEBUG)
	fprintf(stderr,"Listen Socket successfully set\n");
	#endif
	
	/*****************/
	
	extraThreads = 0;
	ErrorServer = 0;
	numClient = 0;
	numThread = 0;
	objStoreSizeByte = 0;
	objStoreSizeFile = 0;
	objStoreSizeDire = -1;
	
	//creazione della directory
	if (mkdir(SERVERDIRECTORY,0777) == -1){
		if (errno != EEXIST){
			perror("CreateDir");
			return -1;
		}
	}
	
	//inizializzazione dati per SIGUSR1
	if (nftw(SERVERDIRECTORY, get_info, 20, 0) == -1) {
     	perror("nftw");
        return -1;
    }
	
	#if defined(DEBUG)
	fprintf(stderr,"Global variables successfully initialized\n");
	#endif
	
	//creazione tabella hash
	if (Create() == -1){
		fprintf(stderr,"Errore Creazione Struttura Dati\n");
		return -1;
	}
	
	#if defined(DEBUG)
	fprintf(stderr,"Hash Table successfully created\n");
	#endif
	
	while(1){
		
		fd_set fds;
		int res;
		struct timespec tv = {0,100000};
		
		FD_ZERO(&fds);
		FD_SET(listenfd,&fds);
		
		//nella pselect rimetto la vecchia maschera per vedere se sono arrivati dei segnali
		res = pselect(listenfd+1,&fds,NULL,NULL,&tv,&oldSet);
		
		if (res < 0 && errno != EINTR){
			perror("pselect");
			ErrorServer = 1;
			break;
		} 
		
		if (sigFlags[0] == 1)
			break;
		
		//Segnale SIGUSR1 ricevuto
		if (sigFlags[1] == 1){
			sigFlags[1] = 0;
			printInfoSUGUSR1();
			continue;
		}
		
		if (res == 0)
			continue;
		
		if (FD_ISSET(listenfd,&fds)){
		
			if ((fdclient = accept(listenfd, (struct sockaddr*) NULL, NULL)) == -1){
				perror("accept");
				ErrorServer = 1;
				break;
			}
		
			if (createThread(fdclient) == -1){
				ErrorServer = 1;
				break;
			}
		}
	}
	
	#if defined(DEBUG)
	fprintf(stderr,"Closing Server ...\n");
	#endif
	
	/*attendo che il numero di thread sia uguale a 0*/
	pthread_mutex_lock(&mutexNumThread);
	
	while(numThread > 0)
		pthread_cond_wait(&condNumThread,&mutexNumThread);
	
	pthread_mutex_unlock(&mutexNumThread);
	
	#if defined(DEBUG)
	fprintf(stderr,"All serving threads are terminated\n");
	#endif
	
	FreeDataStruct();
	
	#if defined(DEBUG)
	fprintf(stderr,"Successfully freed hash table\n");
	#endif
	
	fprintf(stderr, "Main Thread terminated\n");
	
	return 0;
}
