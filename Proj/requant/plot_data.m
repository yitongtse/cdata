function plot_data
    data = load('data');
    F1 = data(:,1);
    DF1 = data(:,2);
    DF2 = data(:,3);	% new result
    DF3 = data(:,4);	% old result
    figure
    hold
    plot(F1, abs(DF2), 'g')
    plot(F1, abs(DF3), 'r:')
%    plot(F1, abs(DF1), 'b:')
    xlabel('Input value')
    ylabel('Absolute quantization noise')
    box
    axis([0 512 0 20])
    legend('Proposed requant method', 'Requant based on TM5')

