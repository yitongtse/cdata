function [H,w] = myfreqz(B,A,N,whole,fs)
%MYFREQZ Frequency response of IIR filter B(z)/A(z).
%    N = number of uniform frequency-samples desired
%    H = returned frequency-response samples (length N)
%    w = frequency axis for H (length N) in radians/sec
%    Compatible with simple usages of FREQZ in Matlab.
%    FREQZ(B,A,N,whole) uses N points around the whole 
%    unit circle, where 'whole' is any nonzero value.
%    If whole=0, points go from theta=0 to pi*(N-1)/N.
%    FREQZ(B,A,N,whole,fs) sets the assumed sampling 
%    rate to fs Hz instead of the default value of 1.
%    If there are no output arguments, the amplitude and 
%    phase responses are displayed (this feature is 
%    Matlab-compatible only). Cannot have poles @ |z|=1.

A = A(:).'; na = length(A); % normalize to row vectors
B = B(:).'; nb = length(B);
if nargin < 3, N = 1024; end
if nargin < 4, whole = 0; end
if nargin < 5, fs = 1; end
Nf = 2*N; if whole, Nf = N; end
w = (2*pi*fs*(0:Nf-1)/Nf)';
H = fft([B zeros(1,Nf-nb)]) ./ fft([A zeros(1,Nf-na)]);
if whole==0, w = w(1:N); H = H(1:N); end

if nargout==0 % Display frequency respone
  if fs==1, flab = 'Frequency (cyles/sample)';
  else, flab = 'Frequency (Hz)'; end
  subplot(2,1,1);
  plot([0:N-1]*fs/N,20*log10(abs(H))); grid;
  xlabel('Frequency (Hz)'); ylabel('Magnitude (dB)');
  subplot(2,1,2);
  plot([0:N-1]*fs/N,angle(H)); grid;
  xlabel('Frequency (Hz)'); ylabel('Phase');
end

