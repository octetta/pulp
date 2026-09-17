/ Classic 808-style Crash Cymbal
N: 66150
T: !N
/ TR-808 6-oscillator metallic cluster
Z: 300 b T

/ Body filter (Bandpass @ 800Hz)
C: 800 1.5
D: C g Z

/ Splash filter (Bandpass @ 3800Hz)
F: 3800 1.5
H: F g Z

/ White noise sizzle (Highpass @ 8000Hz)
I: r T
W: 8000 1.0
Y: W g I

/ Envelopes: long metal, shorter noise
E: e(T*(0-4.0%N))
G: e(T*(0-10.0%N))

/ Mix
M: E*(D*0.5 + H*0.5) + G*Y*0.4

O: 1
M
