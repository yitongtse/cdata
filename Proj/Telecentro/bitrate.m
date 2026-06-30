in = load('telecentro.out');
sz = size(in)
plot(in(2:sz(1),1)/1000, in(2:sz(1), 3) / 1000000);
xlabel('Arrival time (sec)');
ylabel('Bitrate (Mbps)');
axis([0 in(sz(1), 1)/1000 38.7 38.9])
title('Telecentro Time Capture Analysis')


