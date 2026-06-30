function uqam2()
    a = load('/tmp/sim.out');

    figure
    hold

    N = 500000;

    in_tpi = a(:,1);
    dj_tpi = a(:,2);
    lag = a(:,3);

    subplot(1, 3, 1)
    plot(in_tpi, 'r')
    title('Input TPI')
    axis([0 N 0 5000])
    grid

    subplot(1, 3, 2)
    plot(dj_tpi, 'r')
    title('Dejittered TPI')
    axis([0 N 395 398])
    grid

    subplot(1, 3, 3)
    plot(lag, 'r')
    title('Lag')
%    axis([0 N 395 398])
    grid

