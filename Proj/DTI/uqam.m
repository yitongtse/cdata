function uqam()
    a = load('out.stat');

    N = size(a,1)

    figure
    hold

    err_tbo = a(:,1);
    flt_err_tbo = a(:,2);
    freq_adj = a(:,3);
    phase_err = a(:,4);
    ts_jitter = a(:,5);

    subplot(2, 2, 1)
    plot(flt_err_tbo, 'r')
    title('Filtered error TBO (ticks)')
    axis([0 N -0.1 0.1])
    grid

    subplot(2, 2, 2)
    plot(freq_adj, 'r')
    title('Freq adjustment (ppm)')
    axis([0 N -15 15])
    grid

    subplot(2, 2, 3)
    plot(phase_err, 'r');
    title('Phase error (ms)')
    axis([0 N 0 1])
    grid

    subplot(2, 2, 4)
    plot(ts_jitter, 'r');
    title('Timestamp jitter (ns)')
    axis([0 N 0 600])
    grid

