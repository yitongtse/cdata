function c = loopdenom(v, k)
%
% c = LOOPDENOM(v, k)
%
% This function returns the denominator of the closed-loop frequency
% response of a feedback system with gain 'k' and FIR loop filter with
% taps 'v'.
% The loop gain 'k' SHOULD INCLUDE the product of Tts (# seconds
% per timestamp, from the VCO characteristic) and any actual loop gain.

% SCCS info: @(#)loopdenom.m	1.2 4/9/94
% Author: Paul Haskell
% Creation date: 3/22/94

	[row, col] = size(v);
	
	if (row ~= 1),
		error('Please input row vectors only.');
	end
	
	c = zeros(1, col+1);
	
	c(2:col+1) = k .* v;
	c(2) = c(2) - 1;
	c(1) = 1;
end
