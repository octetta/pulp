/ 808-style Crash
N: 66150
T: !N
/ Main decay envelope
E: e(T*(0-6.9%N))
/ Noise decay envelope (slightly shorter)
G: e(T*(0-10.0%N))

/ 6 inharmonic square waves
B: 350*((p 2)%(p 0))
P: +\(N#(B*1.000))
Q: +\(N#(B*1.342))
R: +\(N#(B*1.200))
S: +\(N#(B*1.618))
U: +\(N#(B*1.478))
V: +\(N#(B*1.784))

/ Square wave partials
A: 1 0 0.33 0 0.2
J: P $ A
K: Q $ A
L: R $ A
M: S $ A
X: U $ A
Y: V $ A
Z: J+K+L+M+X+Y

/ Bandpass filter at 3.5kHz (Q=1.5)
F1: 3500 1.5
B1: F1 g Z

/ Bandpass filter at 8kHz (Q=1.5)
F2: 8000 1.5
B2: F2 g Z

/ Mix filtered metal with filtered noise
NOISE: m T
/ Highpass the noise so it's a sizzle (Bandpass at 10kHz)
F3: 10000 1.0
B3: F3 g NOISE

MIX: E*(B1*0.4 + B2*0.6) + G*(B3*0.3)

O: 1
MIX
