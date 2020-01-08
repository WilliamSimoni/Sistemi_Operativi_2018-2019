#if !defined(SIGNALS_H)
#define SIGNALS_H

#include <signal.h>
#include <stdarg.h>

/*
Setta lo stesso handler per i segnali indicati (in totale numSignals). Ritorna 0 in caso di successo, -1 in caso di errore
NOTA: sa_mask e sa_flags dipendono dall'implementazione 
*/
int setHandlerSignals(void (*handler) (int), int numSignals, ...);

/*
Setta la nuova signal mask a set, e memorizza in oldset la vecchia maschera. Ritorna 0 in caso di successo, -1 altrimenti
*/
int setMask(sigset_t set, sigset_t *oldset);

#endif


