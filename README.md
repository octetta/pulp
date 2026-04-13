<img src="IMG_3240.jpeg" width="100">

# `PULP`

> Programmable Universal Logic Pieces

or

> Piles of Unreadable Logic Patches

Nobody likes the pulp...


# Reminders

## go to the parts directory for building

```
cd parts
mkdir build
cd build
```

## features

```
ADSR   # amplitude envelope
AM     # amplitude modulation 
CRUSH  # reduce bit depth 
FILT   # multimode analog-ish filter
FADSR  # filter envelope
FM     # frequency modulation 
PANMOD # stereo panning modulation
PD     # phase distortion 
SAH    # sample rate distortion 
SEQ    # pattern sequencing 
SMOOTHER # volume change smoother 

UDP    # receive skode on UDP
RECORD # multi track wav recording 
BENCH  # random benchmark measures
```

## use the native compiler (enabling features)

```
cmake -DKIT_OPTS="ADSR=1 PD=1 FILT=1 FADSR=1" ..
make
./mini-skred
```

## run a smoke test

From the `parts` directory:

```
make test
```

## forces `zig cc`

```
cmake -DCMAKE_C_COMPILER="zig;cc" ..
make
./mini-skred
```

## cross compile with `zig cc` (x86_64)

```
cmake -DCMAKE_C_COMPILER="zig;cc;-target;x86_64-linux-musl" ..
make
./mini-skred
```

## cross compile with `zig cc` (arm 32-bit)

> this doesn't work yet because the kit/kit_tool needs to be native

```
cmake -DCMAKE_C_COMPILER="zig;cc;-target;arm-linux-gnueabihf" ..
make
./mini-skred
```
