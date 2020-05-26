CFLAGS=-Wall -fPIC -msse3 -mfpmath=sse -O2

OBJFILES=srcs_tcl.o srcs.o srcs_sgs_serial0.o srcs_sgs_serial1.o srcs_sgs_serial2.o fifo.o pqueue.o 

libsrcs.so: $(OBJFILES) Makefile
	$(CC) $(CFLAGS) $(OBJFILES)  -lgsl -lgslcblas -lm -ltclstub8.6 -shared -o libsrcs.so
 
srcs_tcl.o: srcs_tcl.c srcs.h Makefile
	$(CC) $(CFLAGS) -DUSE_TCL_STUBS -c srcs_tcl.c -o srcs_tcl.o

srcs.o: srcs.c srcs.h  Makefile
srcs_sgs_serial0.o: srcs_sgs_serial0.c srcs.h  Makefile	
srcs_sgs_serial1.o: srcs_sgs_serial1.c srcs.h  Makefile	
srcs_sgs_serial2.o: srcs_sgs_serial2.c srcs.h  Makefile	

clean:
	rm -f pqueue.o
	rm -f fifo.o
	rm -f srcs.o
	rm -f srcs_tcl.o
	rm -f srcs_sgs_serial0.o srcs_sgs_serial1.o srcs_sgs_serial2.o
	rm -f libsrcs.so
	rm -f srcs_pwl.o
	rm -f srcs_mfssearch.o
