alpha = 0.01;
fs = 1;
N = 1024;

B = 1;
A = [1 alpha-1];
myfreqz(B, A, 1024, 0, fs)

