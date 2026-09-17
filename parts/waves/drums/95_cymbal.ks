/ Classic 808-style Ride Cymbal
N: 52920
T: !N
/ Tighter, higher tuned metallic cluster
Z: 480 b T

/ Low ping filter (Bandpass @ 1500Hz, Q=3)
C: 1500 3.0
D: C g Z

/ High ping filter (Bandpass @ 5500Hz, Q=3)
F: 5500 3.0
H: F g Z

/ Sharp stick hit noise (Highpass @ 10kHz)
I: m T
W: 10000 1.0
Y: W g I

/ Envelopes: medium metal, very fast click
E: e(T*(0-5.0%N))
G: e(T*(0-40.0%N))

/ Mix
M: E*(D*0.6 + H*0.4) + G*Y*0.6

O: 1
M
