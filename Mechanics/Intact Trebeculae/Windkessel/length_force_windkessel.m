clear all
clc

% Note that this code uses an input q and an output p. In the real system
% this would be an input in dL/dt and the output in F.

% Step size
dt = 0.001;

% Create time vector and and input vector q in [V/s]
t = 0:dt:(10-dt);
q = 15*sin(pi*t/0.8); 

% Reshape sinewave q into the actual input signal
% this is done by removing parts of the sinewave
for i = 1:length(t)
    if ((i>400&&i<801)||(i>2000&&i<2401)||(i>3600&&i<4001)||(i>5200&&i<5601)||(i>6800&&i<7201)||(i>8400&&i<8801))
        q(i) = q(i);
    else q(i) = 0;
    end
end

% Still have to check for less steep ascending dL/dt

% Set initial value for the output p
p0 = 3.45;

% Parameter values for the four elements of the windkessel
R = 1.5;
c = 8;
r = 0.03;
L = 0.02;

% Create the state space matrices
A = [-1/(R*c) 0; 0 -r/L];
B = [1/c; r/L];
C = [1 -r];
D = r;

% Create discrete state space matrices Ad and Bd
Ad = expm(A*dt);
Bd = A\(Ad-eye(length(A)))*B;

% Create empty output vectors
p = zeros(1,length(t));
x = zeros(2,length(t));
x(:,1) = [p0; 0];

% State space calculations
for i = 1:length(t)
    x(:,i+1) = Ad*x(:,i) + Bd*q(i);
    p(i) = C*x(:,i) + D*q(i);
end

% Plot the results
plot(t,p,'c')
xlabel('time [s]')
ylabel('Force [V]')