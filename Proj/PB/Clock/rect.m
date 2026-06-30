function r = rect(n)

% RECT(n)
%
% This function returns an array with 'n' elements, each with value of 1/n.
% This defines a unity-gain FIR rectangular filter.

	r = ones(1,n)/n;
