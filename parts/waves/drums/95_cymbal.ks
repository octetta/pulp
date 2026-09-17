/ Classic Analog Ride Cymbal
N: 52920
T: !N
/ Metallic cluster
Z: 400 b T

/ Bell character (Bandpass @ 2500Hz, moderate Q)
C: 2500 1.5
D: C g Z

/ Wash character (Bandpass @ 6500Hz, wide Q)
F: 6500 1.0
H: F g Z

/ White noise sizzle (Bandpass @ 9000Hz)
I: r T
W: 9000 1.0
Y: W g I

/ Metal envelope (Long decay)
E: e(T*(0-6.0%N))

/ Noise envelope (Slightly faster decay)
G: e(T*(0-10.0%N))

/ Mix
M: E*(D*0.4 + H*0.6) + G*(Y*0.3)

O: 1
M
