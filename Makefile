CC = gcc
AR = ar
CFLAGS += -std=c99 -Wall -g
ARFLAGS = rvs
OPTFLAGS = -O3
LIBSSERVER = -pthread -lSignals -lParams -lData
LIBSCLIENT = -lObjStoreOperation -lParamsClient -pthread
DEBUG = -DDEBUG=1
LIB = ./LIBS
LDFLAGS = -L$(LIB)
INCLUDES = -I$(LIB)
DATADIR = data

TARGETS = server client
CLEANTARGETS = server client $(LIB)/libSignals.a $(LIB)/libParams.a $(LIB)/libData.a $(LIB)/libObjStoreOperation.a $(LIB)/Signals.o $(LIB)/Params.o $(LIB)/Data.o $(LIB)/ObjStoreOperation.o $(LIB)/ParamsClient.o $(LIB)/libParamsClient.a *.log objstore.sock
 
.PHONY = all clean test
.SUFFIXES = .c .h

all: $(TARGETS) 

server: server.c $(LIB)/libSignals.a $(LIB)/libParams.a $(LIB)/libData.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) $(LIBSSERVER)

client: client.c $(LIB)/libObjStoreOperation.a $(LIB)/libParamsClient.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) $(LIBSCLIENT)
	
$(LIB)/libSignals.a: $(LIB)/Signals.o $(LIB)/Signals.h
	$(AR) $(ARFLAGS) $@ $<
	
$(LIB)/libParams.a: $(LIB)/Params.o $(LIB)/Params.h
	$(AR) $(ARFLAGS) $@ $<

$(LIB)/libData.a: $(LIB)/Data.o $(LIB)/Data.h $(LIB)/icl_hash.h
	$(AR) $(ARFLAGS) $@ $<

$(LIB)/libObjStoreOperation.a: $(LIB)/ObjStoreOperation.o $(LIB)/ObjStoreOperation.h
	$(AR) $(ARFLAGS) $@ $<
	
$(LIB)/libParamsClient.a: $(LIB)/ParamsClient.o $(LIB)/Params.h
	$(AR) $(ARFLAGS) $@ $<

$(LIB)/ObjStoreOperation.o: $(LIB)/ObjStoreOperation.c
	$(CC) $(CFLAGS) -pthread $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

$(LIB)/%.o: $(LIB)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

clean: 
	-rm -f $(CLEANTARGETS)
	-rm -rf $(DATADIR)

test: all
	./test.sh
	./testsum.sh
	

