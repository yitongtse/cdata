function uqam3()
    a = load('/tmp/sim.out');

    N = size(a,1)

    figure
    hold

    err_tbo = a(:,1);
    flt_err_tbo = a(:,2);
    freq_adj = a(:,3);
    out_phase_err = a(:,4);
    ts_jitter = a(:,5);
    dj_phase_err = a(:,6);


    subplot(2, 3, 1)
    plot(err_tbo, 'r')
    title('Error TBO (ticks)')
    axis([0 N -10 10])
    grid

    subplot(2, 3, 2)
    plot(flt_err_tbo, 'r')
    title('Filtered error TBO (ticks)')
    axis([0 N -10 10])
    grid

    subplot(2, 3, 3)
    plot(freq_adj, 'r')
    title('Freq adjustment (ppm)')
    axis([0 N -15 15])
    grid

    subplot(2, 3, 4)
    plot(ts_jitter, 'r');
    title('Timestamp jitter (ns)')
    axis([0 N 0 600])
    grid

    subplot(2, 3, 5)
    plot(dj_phase_err, 'r');
    title('Dejitter phase error (ms)')
    axis([0 N -10 10])
    grid

    subplot(2, 3, 6)
    plot(out_phase_err, 'r');
    title('Output phase error (ms)')
    axis([0 N -10 10])
    grid

