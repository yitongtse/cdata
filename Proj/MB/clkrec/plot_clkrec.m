function plot_cr()

    a = load('short.out');

    tp_idx      = a(:,1);
    arvl_time   = a(:,2);
    pcr         = a(:,3);
    out_time    = a(:,4);
    stay_time   = a(:,5);
    bitrate     = a(:,6);
    tbo_in      = a(:,7);
    tbo_est     = a(:,8);
    tbo_err_lpf = a(:,9);
    freq_adj    = a(:,10);


    figure
    hold

    subplot(3, 2, 1)
    plot(tp_idx, pcr, 'r')
    xlabel('TP index')
    ylabel('PCR value')
    grid

    subplot(3, 2, 2)
    plot(tp_idx, freq_adj, 'r')
    xlabel('TP index')
    ylabel('Freq adj (ppm)')
    grid

    subplot(3, 2, 3)
    plot(tp_idx, stay_time, 'r')
    xlabel('TP index')
    ylabel('Stay time')
    grid

    subplot(3, 2, 4)
    plot(tp_idx, bitrate, 'r')
    xlabel('TP index')
    ylabel('Bitrate')
    grid

    subplot(3, 2, 5)
    plot(tp_idx, tbo_est, 'r')
    xlabel('TP index')
    ylabel('Est. TBO')
    grid

