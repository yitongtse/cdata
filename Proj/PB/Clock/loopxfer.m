function l = loopxfer(fir, gain, len, tts)

% loop-f = LOOPXFER(FirTaps, Gain, FftLen, Tts)
%
% This function returns the closed-loop frequency response of a
% PLL feedback system with loop filter 'FirTaps' and feedback gain
% 'Gain'.  'Tts' is the timestamping period.
%

    if (nargin ~= 4)
        error('Wrong number of arguments')
    end

    % Check input filter
    [row, col] = size(fir);
    if (row ~= 1)
	error('1st argument must be a row vector!');
    end

    w = 2 * pi * sqrt(-1) / len;
    l = zeros(1, len);

    for freq = 0 : len-1
        z = exp(freq * w);
        zinv = exp(-freq * w);
        zpow = 1;
        Fz = 0;

        for k = 1 : col
            Fz = Fz + fir(k) * zpow;
            zpow = zpow * zinv;
        end

        numer = tts * gain * Fz;
        denom = z - 1 + numer;

        l(freq+1) = numer / denom;
    end
