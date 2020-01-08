#define _POSIX_C_SOURCE  200112L

#include <Signals.h>

#include <stdarg.h>
#include <string.h>
#include <pthread.h>

//sa_mask = tutti i segnali, opzioni = nessuna
int setHandlerSignals(void (*handler) (int), int numSignals, ...){
	
	va_list ap;
	va_start(ap,numSignals);
	
	int i;
	sigset_t sa_m;	//maschera durante l'esecuzione del gestore
	
	if (sigfillset(&sa_m) == -1)
		return -1;
	
	struct sigaction s;
		
	memset(&s,0,sizeof(s));
		
	s.sa_handler = handler;
	s.sa_mask = sa_m;
			
	for (i = 0; i < numSignals; i++){
		if (sigaction(va_arg(ap,int),&s,NULL) != 0)
			return -1;
	}
	
	va_end(ap);
	
	return 0;
}

int setMask(sigset_t set, sigset_t *oldset){
	if (pthread_sigmask(SIG_SETMASK,&set,oldset) != 0)
		return -1;
	else
		return 0;
}
