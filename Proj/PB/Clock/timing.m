function timing(f)
    a = load('log');

    figure
    hold
    plot(a(:,1), 'r:')
%    plot(a(:,2), 'g')
%    plot(a(:,3), 'b')

%    axis([0 100000 -0.0001 0.00001])
%   legend('LP in', 'LP out')
    xlabel('Time stamp') 
    ylabel('frequency difference (ppm)')
    title('Clock Recovery (freq error = 30 ppm, lowpass 0.0001)')
    grid

