/ 808-style Ride/Cymbal
N: 52920
T: !N
E: e(T*(0-6.9%N))
G: e(T*(0-25.0%N))

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

C: 800 2.0
D: C g Z
F: 4200 2.0
H: F g Z

I: m T
W: 6000 1.0

O: 1
E*(D*0.5 + H*0.5) + G*(W g I)*0.4
