    N = 1024;
    alpha = .001;
    A = [1 alpha-1];
    B = alpha;
    x = zeros(1, N);
    x(1) = 1;
    y = filter(B, A, x);
    Hz = fft(y, 1024);

    figure
    loglog(abs(Hz))
