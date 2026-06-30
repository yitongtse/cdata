function q = pllacquire(FiltTaps, Gain)
%	q = PLLACQUIRE(FiltTaps, Gain)
%
% Returns the response (at the VCO input) of a PLL system fed by
% a ramp.

% SCCS info: @(#)pllacquire.m	1.2 4/13/94
% Author: Paul Haskell
% Creation date: 4/9/94


%
%	Need a fcn that calculates z-xform of vco input, if PLL is fed a
%	step (NOT A RAMP)!!!

% Calculate PLL step response.
	p = loopxfer(FiltTaps, Gain, 3);
	q = ifft(p);

% Integrate to get ramp response.
	[m, n] = size(q);
	q(1) = real(q(1));
	for i = 2:n,
		q(i) = q(i-1) + real(q(i));
	end
end
