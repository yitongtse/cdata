function r = rect(c)
%
% r = RECT(c)
%
% This function returns an array with 'c' elements, each with value
% 1.0 / 'c'.  This defines a unity-gain FIR rectangular filter,
% for example

% SCCS info: @(#)rect.m	1.1 3/23/94
% Author: Paul Haskell
% Creation date: 3/22/94

	r = zeros(1, c);
	r = r + 1;
	r = r / c;
end
