#if !defined(PARS_H)
#define PARS_H

#include <IOop.h>

//struct per indicare i parametri dell'operazione inviati dal client
typedef struct pars_{
	char op_name [16];		//nome dell'operazione
	char name [BUFSIZE];	//nome del file
	long len;				//lunghezza del file (usato solo in caso di store)
}pars;

typedef struct bufpars_{
	char data[BUFSIZE];		//buffer in cui vengono memorizzati i dati letti dal socket
	int start;				//inizio zona buffer non significativa per l'handler
	int end;				//fine zona buffer con caratteri letti
}bufpars;

/*
//REQUIRES: param != NULL
//EFFECTS: modifica param in modo che nei suoi elementi siano memorizzati le informazioni contenuto nell'header inviato dal client
//RETURNS: ritorna -1 se param == NULL, formattazione errata dell'header o altri eventuali errori. Ritorna 0 altrimenti
*/
int getPars(pars* param, long fd,bufpars* outOfHeader);

#endif
