function l = loopxfer(x, gain, R, len, tts)
%
% loop-f = LOOPXFER(InputFirTaps, Gain, Response, FftLen, Tts)
%
% This function returns the closed-loop frequency response of a
% PLL feedback system with loop filter 'InputFirTaps' and feedback gain
% 'Gain'. 'Tts' is the number of seconds per timestamps.
%
% The 'Response' variable determines what transfer function is returned:
%		R			Xfer Fcn
%		--		---------------------
%		1		clock frequency error
%		2		clock slew-rate
%		3		acquisition time

% Author: Paul Haskell
% Creation date: 3/22/94
% SCCS info: @(#)loopxfer.m	1.3 4/13/94


% Check inputs.
	if (nargin < 2),
		error('incorrect usage: see help loopxfer');
	end
	if (nargin < 3),
		R = 1;
	end
	if (nargin < 4),
		len = 2048;
	end
	if (nargin < 5),
		tts = 0.1;
	end

% Check input filter.
	[row, col] = size(x);
	if (row ~= 1),
		if (col == 1),
			col = row;
		else
			error('Must enter a row vector!');
		end
	end


% 'l' holds the output.
	l = zeros(1, len);
	norm = 2 * pi * sqrt(-1) / len;

% Calculate the z xform at equally spaced points around the unit circle.
	for freq = 1:len,
		z = exp((freq-1)*norm);
		zinv = exp((1-freq)*norm);
		zpow = 1;
		numer = 0;
		for k = 1:col,
			numer = numer + x(k) * zpow;
			zpow = zpow * zinv;
		end

		numer = numer * gain;
		denom = z - 1 + tts*numer;

		if R == 1					% For clock error measurements
			numer = numer * (z-1);
		elseif R == 2				% For clock slew measurements
			denom = denom * tts * z;
			numer = numer * (z-1);
		else						% For acquisition measurements
			numer = numer * z;
		end

		l(freq) = numer / denom;
	end
end
