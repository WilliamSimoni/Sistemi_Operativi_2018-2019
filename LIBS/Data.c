#include <Data.h>
#include <icl_hash.h>
#include <unistd.h>
#include <pthread.h>

/**DATA_STRUCT*/
#define NBUCKETS 128

static pthread_mutex_t mutexHash = PTHREAD_MUTEX_INITIALIZER;

static icl_hash_t *hash_table = NULL;

//crea la struttura dati, ritorna 0 in caso di successo, -1 in caso di fallimento.
int Create(){
	hash_table = icl_hash_create(NBUCKETS, NULL, NULL);
    if (!hash_table) {
		fprintf(stderr, "Errore nella creazione della table hash\n");
		return -1;
    }
    return 0;
}

//ritorna 0 se l'inserimento ha avuto successo. -1 se el è già presente.
int Insert(char* el){
	pthread_mutex_lock(&mutexHash);
	if (hash_table && !icl_hash_find(hash_table, el)){
		icl_hash_insert(hash_table, el, (void*)el); 
	} 
	else{
		pthread_mutex_unlock(&mutexHash);
		return -1;
	}
	pthread_mutex_unlock(&mutexHash);
	return 0;
}


int Delete(char* el){
	pthread_mutex_lock(&mutexHash);
	int r = icl_hash_delete(hash_table,el,NULL,free);
	pthread_mutex_unlock(&mutexHash);
	return r;
}

int FreeDataStruct(){
	return icl_hash_destroy(hash_table, NULL, free);
}
/***************/

