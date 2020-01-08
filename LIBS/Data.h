#if !defined(DATA_STRUCT_H)
#define DATA_STRUCT_H

//crea la struttura dati, ritorna 0 in caso di successo, -1 in caso di fallimento.
int Create();

//ritorna 0 se l'inserimento ha avuto successo. -1 se el è già presente.
int Insert(char* el);

//elimina el dalla struttura dati
int Delete(char* el);

//libera tutto lo spazio occupato dalla struttura dati
int FreeDataStruct();

#endif
