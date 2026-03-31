<img src="pulp.png" width="200">

# PULP

> produce-ultimate-limited-program

Nobody likes the pulp...


# Reminders

## go to the parts directory for building

```
cd parts
mkdir build
cd build
```

## features

ADSR
AM
BENCH
CRUSH
FILT
FADSR
FM
GLISS
PANMOD
PD
SAH
SEQ
SMOOTHER

UDP
RECORD
BENCH
MAXI

## use the native compiler (enabling features)

```
cmake -DKIT_OPTS="ADSR=1 PD=1 FILT=1 FADSR=1" ..
make
./mini-skred
```

## forces `zig cc`

```
cmake -DCMAKE_C_COMPILER="zig;cc" ..
make
./mini-skrec
```

## cross compile with `zig cc` (x86_64)

```
cmake -DCMAKE_C_COMPILER="zig;cc;-target;x86_64-linux-musl" ..
make
./mini-skrec
```

## cross compile with `zig cc` (arm 32-bit)

> this doesn't work yet because the kit/kit_tool needs to be native

```
cmake -DCMAKE_C_COMPILER="zig;cc;-target;arm-linux-gnueabihf" ..
make
./mini-skrec
```

