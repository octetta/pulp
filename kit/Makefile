all: mini-skred

synth.o: synth.c synth.h synth-types.h
	$(CC) $(COPTS) -c $< -o $@

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

mini-skred : synth.o skqueue.o seq.o util.o skode.o synth-alloc.o miniwav.o miniaudio.o mini-skred.c
	$(CC) synth.o util.o ands.o skqueue.o seq.o skode.o synth-alloc.o miniwav.o miniaudio.o mini-skred.c -o mini-skred -lm -lpthread
