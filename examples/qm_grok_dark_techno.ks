# ----------------------------------------------------------------------
# Queen Mary – Berlin / Dark Techno version
# Purcell → Carlos → concrete basement
# ----------------------------------------------------------------------

M 138 16

# Short, dark, controlled delay
DT 1 140
DL 1 - - 4 0 1 5
DD 1 9 4

# ======================================================================
# K-SYNTH DRUMS
# ======================================================================
[sk/drums-kick.ks]   /ks 1 k>d d>r /r100
[sk/drums-snare.ks]  /ks 1 k>d d>r /r101
[sk/drums-chh.ks]    /ks 1 k>d d>r /r102
[sk/drums-ohh.ks]    /ks 1 k>d d>r /r103
[sk/drums-clap.ks]   /ks 1 k>d d>r /r104
[sk/drums-rim.ks]    /ks 1 k>d d>r /r105

# ======================================================================
# VOICES
# ======================================================================

# Kick – heavy, long tail, low
v 0 w100 f440 a12

# Snare / Clap / Rim
v 1 w101 f440 a7
v 5 w104 f440 a6
v 4 w105 f440 a5

# Hats – darker, lower level
v 2 w102 f440 a2.8
v 3 w103 f440 a3.5

# Dark stab ensemble (Voices 6–9)
# Lower cutoff, higher resonance, slower attack, longer decay
[stab] : v $$0 w 19 a 0.38 t 0.012 0.28 0.25 0.18 J 1 K 680 Q 3.2 ;
6 stab
7 stab
8 stab
9 stab

v 6 N 0 0
v 7 N 0 11
v 8 N 0 -9
v 9 N 0 -4

v 6 g 0.02
v 7 g 0.025
v 8 g 0.018
v 9 g 0.03

# Slightly more staggered, darker bloom
v 6 L 0.00
v 7 L 0.018
v 8 L 0.032
v 9 L 0.009

v 6 p 0 r 1 ds 4
v 7 p 0 r 1 ds 4
v 8 p 0 r 1 ds 4
v 9 p 0 r 1 ds 4

v 6 n 52
v 7 n 47
v 8 n 40
v 9 n 28

# ======================================================================
# MACROS – same harmony, transposed down for darker register
# ======================================================================
[off] : v6 l0 v7 l0 v8 l0 v9 l0 ;

# Section A (down a 12th / octave+5th feel)
[SAa] : v6 n52 l1 v7 n47 l1 v8 n40 l1 v9 n28 l1 ;
[SAb] : v6 n53 l1 v7 n45 l1 v8 n41 l1 v9 n38 l1 ;
[SAc] : v6 n53 l1 v7 n45 l1 v8 n40 l1 v9 n33 l1 ;
[SAd] : v6 n52 l1 v7 n47 l1 v8 n40 l1 v9 n37 l1 ;
[SAe] : v6 n52 l1 v7 n45 l1 v8 n40 l1 v9 n33 l1 ;
[SAf] : v6 n57 l1 v7 n52 l1 v8 n45 l1 v9 n33 l1 ;
[SAg] : v6 n48 l1 v7 n43 l1 v8 n40 l1 v9 n36 l1 ;
[SAh] : v6 n48 l1 v7 n43 l1 v8 n40 l1 v9 n36 l1 ;
[SAi] : v6 n57 l1 v7 n53 l1 v8 n45 l1 v9 n29 l1 ;
[SAj] : v6 n57 l1 v7 n53 l1 v8 n45 l1 v9 n41 l1 ;
[SAk] : v6 n56 l1 v7 n52 l1 v8 n47 l1 v9 n40 l1 ;
[SAl] : v6 n56 l1 v7 n52 l1 v8 n47 l1 v9 n40 l1 ;
[SAm] : v6 n57 l1 v7 n52 l1 v8 n47 l1 v9 n42 l1 ;

# Section B
[SBa] : v6 n59 l1 v7 n55 l1 v8 n47 l1 v9 n43 l1 ;
[SBb] : v6 n57 l1 v7 n52 l1 v8 n45 l1 v9 n33 l1 ;
[SBc] : v6 n54 l1 v7 n50 l1 v8 n45 l1 v9 n35 l1 ;
[SBd] : v6 n55 l1 v7 n52 l1 v8 n47 l1 v9 n28 l1 ;
[SBe] : v6 n52 l1 v7 n48 l1 v8 n43 l1 v9 n36 l1 ;
[SBf] : v6 n47 l1 v7 n43 l1 v8 n38 l1 v9 n38 l1 ;
[SBg] : v6 n55 l1 v7 n52 l1 v8 n48 l1 v9 n36 l1 ;
[SBh] : v6 n53 l1 v7 n48 l1 v8 n45 l1 v9 n29 l1 ;
[SBi] : v6 n50 l1 v7 n47 l1 v8 n43 l1 v9 n31 l1 ;
[SBj] : v6 n52 l1 v7 n48 l1 v8 n43 l1 v9 n36 l1 ;
[SBk] : v6 n52 l1 v7 n48 l1 v8 n43 l1 v9 n36 l1 ;
[SBl] : v6 n55 l1 v7 n50 l1 v8 n47 l1 v9 n31 l1 ;
[SBm] : v6 n57 l1 v7 n52 l1 v8 n45 l1 v9 n33 l1 ;
[SBn] : v6 n57 l1 v7 n53 l1 v8 n45 l1 v9 n38 l1 ;
[SBo] : v6 n56 l1 v7 n50 l1 v8 n47 l1 v9 n40 l1 ;
[SBp] : v6 n57 l1 v7 n52 l1 v8 n45 l1 v9 n33 l1 ;
[SBq] : v6 n50 l1 v7 n45 l1 v8 n41 l1 v9 n38 l1 ;
[SBr] : v6 n52 l1 v7 n47 l1 v8 n40 l1 v9 n28 l1 ;

# ======================================================================
# PATTERN 1 – Drums (harder, darker)
# ======================================================================
y1
% 1
[v0 l1] x0
[v2 l1] x1
[v2 l1] x2
[v4 l1] x3               # rim
[v0 l1 v1 l1] x4         # kick + snare
[v2 l1] x5
[v3 l1] x6               # open
[v2 l1] x7
[v0 l1] x8
[v2 l1] x9
[v2 l1] x10
[v4 l1] x11
[v0 l1 v5 l1] x12        # kick + clap
[v2 l1] x13
[v3 l1] x14
[v2 l1] x15
[#] x15

# ======================================================================
# PATTERN 2 – Section A stabs (longer gates, darker)
# ======================================================================
y2
% 1
[SAa] x0
[off] x3
[SAb] x4
[off] x7
[SAc] x8
[off] x11
[SAd] x12
[off] x15
[SAe] x16
[off] x19
[SAf] x20
[off] x23
[SAg] x24
[off] x27
[SAh] x28
[off] x31
[#] x31

# ======================================================================
# PATTERN 3 – Section B stabs
# ======================================================================
y3
% 1
[SBa] x0
[off] x3
[SBb] x4
[off] x7
[SBc] x8
[off] x11
[SBd] x12
[off] x15
[SBe] x16
[off] x19
[SBf] x20
[off] x23
[SBg] x24
[off] x27
[SBh] x28
[off] x31
[SBi] x32
[off] x35
[SBj] x36
[off] x39
[SBk] x40
[off] x43
[SBl] x44
[off] x47
[#] x47

# ======================================================================
# MASTER
# ======================================================================
y0
% 16
[ /z 1 1 ] x0
[ /z 2 1 ] x4
[ /z 2 0 /z 2 1 ] x12
[ /z 2 0 /z 3 1 ] x20
[ /z 3 0 /z 3 1 ] x32
[ /z 3 0 /z 1 0 off ] x48
[ /z 0 0 ] x50
