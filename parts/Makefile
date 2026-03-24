CC = gcc

all: kit mini-skred

reset:
	touch *.kit

FEAT = \
  PD=0 \
  FILT=1 \
  FADSR=1 \
  #

# KIT = ./kit --include features.kit
KIT = ./kit $(FEAT)

kit: kit.c
	$(CC) kit.c -o kit

synth.c: synth.kit
	$(KIT) < synth.kit > synth.c

synth.o: synth.c synth.h synth-types.h
	$(CC) $(COPTS) -c $< -o $@

skode.c: skode.kit
	$(KIT) < skode.kit > skode.c

skode.o: skode.c skode.h ands.h synth.c ands.o
	$(CC) $(COPTS) -DMINI -Wno-multichar -c $< -o $@

seq.o: seq.c seq.h
	$(CC) $(COPTS) -DMINI -c $< -o $@

ands.o: ands.c ands.h
	$(CC) $(COPTS) -c $< -o $@

miniwav.o: miniwav.c miniwav.h
	$(CC) $(COPTS) -c $< -o $@

synth-alloc.o: synth-alloc.c
	$(CC) $(COPTS) -c $< -o $@

skqueue.o: skqueue.c
	$(CC) $(COPTS) -c $< -o $@

util.o: util.c
	$(CC) $(COPTS) -c $< -o $@

mini-skred.c: mini-skred.kit
	$(KIT) < mini-skred.kit > mini-skred.c

mini-skred.o: mini-skred.c

mini-skred : synth.o skqueue.o seq.o util.o skode.o synth-alloc.o miniwav.o miniaudio.o mini-skred.o
	$(CC) synth.o util.o ands.o skqueue.o seq.o skode.o synth-alloc.o miniwav.o miniaudio.o mini-skred.o -o mini-skred -lm -lpthread

clean:
	touch *.kit
	rm *.o
