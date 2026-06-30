


N = 100000;
drift = -50 * 10^(-6);
jit = 100 * 10^(-3);

% drift = 0 * 10^(-6);
% jit = 0 * 10^(-3);


tmp = 100*10^(-3)*rand(N, 1);
t1 = tmp;
for n=2:N
    t1(n) = t1(n-1) + tmp(n);
end

t2 = (1 + drift) * t1 + jit * (rand(N,1) -0.5);  

rd1 = floor(rand(1) * 100000);
rd2 = floor(rand(1) * 100000);

send = floor(t1 * 27*10^6) + rd1;
arrv = floor(t2 * 27*10^6) + rd2;


% -------------------

mm = N - 1;
x(1:mm, 1) = 0;
y(1:mm, 1) = 0;

a(1:mm, 1) = 0;
yo(1:mm, 1) = 0; 
err(1:mm, 1) = 0; 

xx(1:mm, 1) = 0;
yy(1:mm, 1) = 0; 


alpha = 2^(-12);  % originally 12

% k0 = 2^(-8);
% k0 = 2^(-11)/ max(dx);
k0 = 2^(-28);
aprev = 0;

dddd(1:N, 1) = 0;  % delay: srrv - send
dl(1:N, 1) = 0;
fdl(1:N, 1) = 0;
deliv(1:N, 1) = 0; 
deliv(1) = arrv(1); 

da(1:N-1, 1) = 0;

dl_scale = 2^0; % 32 : overkill; 16 OK; 1 also OK?
 
for kk = 2 : N
   
   dl(kk) = dl(kk-1) + arrv(kk) -  arrv(kk-1) - send(kk) + send(kk-1);
   dddd(kk,1) = dl(kk);
   
   % yy = (dl(kk)*dl_scale - fdl(kk-1)) * alpha;
   % fdl(kk, 1) = fdl(kk-1) + yy;
   
   yy = floor( (dl(kk)*dl_scale - fdl(kk-1)) * alpha );
   fdl(kk, 1) = fdl(kk-1) + yy;

   n = kk - 1;
   x(n) = send(kk) - send(kk-1);
   gap = x(n);
   y(n) = yy;
     
   
   % fprintf(1, '%d of %d\r', n, mm);
   yo(n) =  aprev * x(n) ;
   err(n) = y(n) - yo(n)*dl_scale;
  

   da(n) = k0 * err(n) / dl_scale;

   a(n) = aprev + da(n);

   aprev = a(n);
   
   
   deliv(kk) = deliv(kk-1) + x(n) + yo(n);
   
end

% return;

subplot(2,2,1);
plot(arrv - send);
xlabel('RTP Packet Numbers');
ylabel('arrv - send');


subplot(2,2,2);
plot(a);
xlabel('RTP Packet Numbers');
ylabel('Tracked Drifting Rate');

subplot(2,2,3);
plot(deliv - arrv);
xlabel('RTP Packet Numbers');
ylabel('deliv - arrv');


subplot(2,2,4);
plot(err);
xlabel('RTP Packet Numbers');
ylabel('Tracking Error');



figure;



delay1 = arrv - send;
delay2 = deliv - send;
mmm = size(arrv, 1);
delta1 = delay1(2:mmm, 1) - delay1(1:mmm-1, 1);
delta2 = delay2(2:mmm, 1) - delay2(1:mmm-1, 1);


subplot(2,2,1);
plot(delay1);
xlabel('RTP Packet Numbers');
ylabel('delay1 = arrv - send');


subplot(2,2,2);
plot(delta1);
xlabel('RTP Packet Numbers');
ylabel('delta1 = delta of delay1');

subplot(2,2,3);
plot(delay2);
xlabel('RTP Packet Numbers');
ylabel('delay2 = deliv - send');


subplot(2,2,4);
plot(delta2);
xlabel('RTP Packet Numbers');
ylabel('delta2 = delta of delay2');



% return;



