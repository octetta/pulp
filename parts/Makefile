CC = gcc -g
COPTS = -g

all: kit mini-skred

reset:
	touch *.kit

FEAT = #

yFEAT =      \
  ADSR=1     \
  #

xFEAT =      \
  ADSR=1     \
  AM=1       \
  BENCH=1    \
  CRUSH=1    \
  FADSR=1    \
  FILT=1     \
  FM=1       \
  GLISS=1    \
  PANMOD=1   \
  PD=1       \
  SAH=1      \
  SEQ=1      \
  SMOOTHER=1 \
  UDP=0      \
  #

# KIT = ./kit --include features.kit
KIT = ./kit $(FEAT)

kit: kit.c
	$(CC) kit.c -o kit

synth.h: synth.h.kit
	$(KIT) < synth.h.kit > synth.h

synth.c: synth.kit
	$(KIT) < synth.kit > synth.c

synth.o: synth.c synth.h synth-types.h
	$(CC) $(COPTS) -c $< -o $@

synth.kit: synth.h.kit

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

synth-state.h: synth-state.h.kit
	$(KIT) < synth-state.h.kit > synth-state.h

synth-alloc.c: synth-alloc.kit synth-state.h
	$(KIT) < synth-alloc.kit > synth-alloc.c

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
	rm -f *.o
