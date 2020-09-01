CC=g++
#CCFLAGS=-g3 -Wall -w -pedantic -fmessage-length=0 -fno-stack-protector -D_FORTIFY_SOURCE=0 -pie -fPIE
CCFLAGS=-g3 -Wall -w -pedantic -fmessage-length=0
#CCFLAGS=-g3 -Wall -w -pedantic -fmessage-length=0 -fno-stack-protector 
#CCFLAGS=-g3 -Wall -w -pedantic -fmessage-length=0 -fno-stack-protector -fPIC

O_FILES = codes.o MersenneRNG.o util.o

codes : $(O_FILES)
	$(CC) $(CCFLAGS) -o codes $(O_FILES)

# dependency stuff, see: http://www.cs.berkeley.edu/~smcpeak/autodepend/autodepend.html
# pull in dependency info for *existing* .o files
-include $(O_FILES:.o=.d)

.SUFFIXES : .o .c

.c.o :  
	$(CC) $(CCFLAGS) -c $<
	$(CC) -MM $(CFLAGS) $*.c > $*.d

clean:
	rm -f codes *.o *.d
