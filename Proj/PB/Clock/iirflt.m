% This program is used to find the minimum gain factor beta
% for the 2-tap IIR filter to have a stable closed loop system
%   alhpa: filter parameter
%   beta = K * T
%   K: loop gain
%   T: sampling period

alpha = 0.001
beta = 0.0001

A = [1 alpha*beta+alpha-2 1-alpha];
B = [0 alpha*beta];
R = roots(A);
'Max root amplitude:'
max(abs(R))

% Plot of pole locations
figure
polar(angle(R), abs(R), 'ro')

% Plot of frequency response
figure
na = length(A);
nb = length(B);
N = 1024;
fs = 1;
Nf = 2*N;
w = (2*pi*fs*(0:Nf-1)/Nf)';
H = fft(B, Nf) ./ fft(A, Nf);
w = w(1:N); H = H(1:N);

subplot(2,1,1);
% semilogx([0:N-1]*fs/N, 20*log10(abs(H)));
semilogx([0:N-1]*fs/N, abs(H));
grid; xlabel('Frequency (Hz)'); ylabel('Magnitude (dB)');

subplot(2,1,2);
plot([0:N-1]*fs/N, angle(H));
grid; xlabel('Frequency (Hz)'); ylabel('Phase');

'Max gain:'
max(abs(H))
