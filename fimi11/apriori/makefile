#-----------------------------------------------------------------------
# File    : makefile
# Contents: build FIMI 2003 workshop contribution (apriori)
# Author  : Christian Borgelt
# History : 15.08.2003 file created from apriori makefile
#           20.08.2003 closed and maximal item set miner added
#           20.09.2003 adapted to new submission rules
#-----------------------------------------------------------------------
CC      = gcc
CFLAGS  = -ansi -Wall -pedantic -DNDEBUG -O3
# CFLAGS  = -ansi -Wall -pedantic -DNDEBUG -O3 -DQUIET
# CFLAGS  = -ansi -Wall -pedantic -g

HDRS    = vecops.h symtab.h tfscan.h tract.h istree.h
OBJS    = vecops.o nimap.o  tfscan.o tract.o istree.o

#-----------------------------------------------------------------------
# Build Program
#-----------------------------------------------------------------------
all:       fim_all fim_closed fim_maximal

fim_all:      $(OBJS) apriori.o makefile
	$(CC) $(OBJS) apriori.o -o $@

fim_closed:   $(OBJS) closed.o makefile
	$(CC) $(OBJS) closed.o -o $@

fim_maximal:  $(OBJS) maximal.o makefile
	$(CC) $(OBJS) maximal.o -o $@

#-----------------------------------------------------------------------
# Main Program
#-----------------------------------------------------------------------
apriori.o: tract.h istree.h symtab.h
apriori.o: apriori.c makefile
	$(CC) $(CFLAGS) -c apriori.c -o $@

closed.o:  tract.h istree.h symtab.h
closed.o:  apriori.c makefile
	$(CC) $(CFLAGS) -DCLOSED -c apriori.c -o $@

maximal.o: tract.h istree.h symtab.h
maximal.o: apriori.c makefile
	$(CC) $(CFLAGS) -DMAXIMAL -c apriori.c -o $@

#-----------------------------------------------------------------------
# Item and Transaction Management
#-----------------------------------------------------------------------
tract.o:   tract.h symtab.h
tract.o:   tract.c makefile
	$(CC) $(CFLAGS) -c tract.c -o $@

#-----------------------------------------------------------------------
# Frequent Item Set Tree Management
#-----------------------------------------------------------------------
istree.o:  istree.h tract.h
istree.o:  istree.c makefile
	$(CC) $(CFLAGS) -c istree.c -o $@

#-----------------------------------------------------------------------
# Vector Operations
#-----------------------------------------------------------------------
vecops.o:   vecops.h
vecops.o:   vecops.c makefile
	$(CC) $(CFLAGS) -c vecops.c -o $@

#-----------------------------------------------------------------------
# Symbol Table Management
#-----------------------------------------------------------------------
nimap.o:    symtab.h vecops.h
nimap.o:    symtab.c makefile
	$(CC) $(CFLAGS) -DNIMAPFN -c symtab.c -o $@

#-----------------------------------------------------------------------
# Table File Scanner Management
#-----------------------------------------------------------------------
tfscan.o:   tfscan.h
tfscan.o:   tfscan.c makefile
	$(CC) $(CFLAGS) -c tfscan.c -o $@

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
clean:
	rm -f *.o *~ *.flc core fim_all fim_closed fim_maximal
