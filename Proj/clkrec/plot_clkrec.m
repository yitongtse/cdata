% Copyright (c) 2007 by Cisco Systems, Inc.
% All rights reserved.

function plot_cr(file)

    a = load(file);

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

    time = (out_time - out_time(1,1)) / 1000;
    pcr = pcr - pcr(1,1);
    tbo_est = tbo_est - tbo_est(1,1);

    figure
    hold

    subplot(3, 2, 1)
    plot(time, pcr, 'r')
    xlabel('Time (sec)')
    ylabel('PCR (ms)')
    grid

    subplot(3, 2, 2)
    plot(time, freq_adj, 'r')
    xlabel('Time (sec)')
    ylabel('Freq adj (ppm)')
    grid

    subplot(3, 2, 3)
    plot(time, bitrate, 'r')
    xlabel('Time (sec)')
    ylabel('Bitrate (bps)')
    grid

    subplot(3, 2, 4)
    plot(time, tbo_est, 'r')
    xlabel('Time (sec)')
    ylabel('Est. TBO (ms)')
    grid

    subplot(3, 2, 5)
    plot(time, stay_time, 'r')
    xlabel('Time (sec)')
    ylabel('Stay time (ms)')
    grid

    subplot(3, 2, 6)
    plot(time, tbo_err_lpf, 'r')
    xlabel('Time (sec)')
    ylabel('LPF TBO ERR')
    grid

