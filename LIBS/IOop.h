#if !defined(IOOP_H)
#define IOOP_H

#include <unistd.h>
#include <errno.h>

//legge al più max caratteri. Esegue una singola lettura se non vengono ricevuti segnali.
static inline int readst(long fd, void *buf, size_t max){
	int r;
	char *bufptr = (char*) buf;
	
	while((r = read((int) fd, bufptr, max)) == -1){
		if (errno == EINTR) 
			continue;
		else
			return -1;
	}
	
	return r;
}

/*
	Legge size byte da fd e li memorizza e buf.
	REQUIRES: dimensione di buf >= size
	EFFECTS: scrive size byte in buf.
	RETURNS: ritorna -1 in caso di errore della read. Altrimenti ritorna 0 se ha incontrato l'EOF. Ritorna size altrimenti.
*/
static inline int readn(long fd, void *buf, size_t size){
	size_t left = size;
	int r;
	char *bufptr = (char*) buf;
	
	while(left>0){
		if ((r = read((int) fd, bufptr, left)) == -1){
			if (errno == EINTR) continue;	//in caso di segnale non mascherato che interrompe la read
			return -1;
		}
		
		if (r == 0) return 0; //raggiunto EOF
		
		left -= r;
		bufptr += r;
		
	}
	
	return size;
}

static inline int writen(long fd, void *buf, size_t size){
	size_t left = size;
	int r;
	char *bufptr = (char*) buf;
	
	while(left>0){
		if ((r = write((int) fd, bufptr, left)) == -1){
			if (errno == EINTR) continue;
			return -1;	
		}
		
		if (r==0) return 0;
		left -= r;
		bufptr += r;
	}	
	
	return 1;
}

#if !defined BUFSIZE
#define BUFSIZE 512
#endif

#if !defined BUFSIZEWRFILE
#define BUFSIZEWRFILE 2048
#endif

#endif
