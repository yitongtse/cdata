function t = pllnoise(FiltTaps, Gain)
%	T = PLLNOISE(FiltTaps, Gain)
%
% Returns integral of the power spectral density of the VCO input of
% a PLL system. The PLL uses an FIR loop filter with taps 'FiltTaps'
% and a loop gain of 'Gain'.

	p = loopxfer(FiltTaps, Gain, 1);
	[r, c] = size(p);

	t = (p * p') / c;
end
