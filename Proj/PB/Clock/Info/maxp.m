function m = maxp(filt, k)
%
% m = MAXP(filt, k)
%
% This function returns the magnitude of the largest pole of the
% closed-loop frequency response of the feedback system with gain 'k'
% and FIR loop filter with taps 'filt'.

% SCCS info: @(#)maxp.m	1.1 3/23/94
% Author: Paul Haskell
% Creation date: 3/22/94

	c = loopdenom(filt, k);
	r = roots(c);
	m = max(abs(r));
end
