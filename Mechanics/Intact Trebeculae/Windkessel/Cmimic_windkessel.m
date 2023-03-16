clear all
clc

dt = 0.001;

t = 0:dt:(10-dt);
q = 15*sin(pi*t/0.8); 
% q = 15*sin(pi*t/1.6);

for i = 1:length(t)
    if ((i>400&&i<801)||(i>2000&&i<2401)||(i>3600&&i<4001)||(i>5200&&i<5601)||(i>6800&&i<7201)||(i>8400&&i<8801))
%     if ((i>800&&i<1601)||(i>4000&&i<4801)||(i>7200&&i<8001))
        q(i) = q(i);
    else q(i) = 0;
    end
end

% Still have to check for less steep ascending dL/dt

p0 = 3.4;

A1 = 0.999916670138793; %0.9999084444;
A2 = 0.998501124437711; %0.9985045930;
B1 = 1.249947918112726e-04; %1.22414e-04;
B2 = 0.001498875562289; %0.00149540696;
C1 = 1;
C2 = -0.03;
D  = 0.03;

p  = zeros(1,length(t));
x1 = zeros(1,length(t));
x2 = zeros(1,length(t));
x1(1) = p0;
x2(1) = 0;

for i = 1:length(t)
    x1(i+1) = A1*x1(i) + B1*q(i);
    x2(i+1) = A2*x2(i) + B2*q(i);
    p(i) = C1*x1(i) + C2*x2(i) + D*q(i);
end

plot(t,p,'m')
xlabel('time [s]')
ylabel('Force [V]')