waveform=csvread('preamble.csv',2,0);
plot(waveform(:,1), waveform(:,2))
% axis([-2e-4 2e-4 -0.5 1.5])