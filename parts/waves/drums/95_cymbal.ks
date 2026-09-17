/ 808-style Ride/Cymbal
N: 52920
T: !N
/ Main decay envelope
E: e(T*(0-6.9%N))
/ Fast attack/decay for stick click
G: e(T*(0-25.0%N))

/ Base tuning
B: 450*((p 2)%(p 0))
P: +\(N#(B*1.000))
Q: +\(N#(B*1.342))
R: +\(N#(B*1.200))
S: +\(N#(B*1.618))
U: +\(N#(B*1.478))
V: +\(N#(B*1.784))

A: 1 0 0.33 0 0.2
J: P $ A
K: Q $ A
L: R $ A
M: S $ A
X: U $ A
Y: V $ A
Z: J+K+L+M+X+Y

/ Bandpass 1 at 800Hz
F1: 800 2.0
B1: F1 g Z

/ Bandpass 2 at 4200Hz
F2: 4200 2.0
B2: F2 g Z

/ Highpass noise for stick impact
NOISE: m T
F3: 6000 1.0
B3: F3 g NOISE

MIX: E*(B1*0.5 + B2*0.5) + G*(B3*0.4)

O: 1
MIX
