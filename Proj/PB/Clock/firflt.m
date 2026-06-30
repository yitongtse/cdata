% This program is used to find the minimum gain factor A for the N-tap
% average filter to have a stable closed loop system

N = 128;
A = 16384;
a = [A 1-A ones(1, N-1)];
R = roots(a);
max_amp = max(abs(R))

% Plot of pole locations
figure
polar(angle(R), abs(R), 'ro')

% Plot of frequency response
figure
b = ones(1, N);
myfreqz(b,a);
