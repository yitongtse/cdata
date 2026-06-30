function plotClkRec()
    load clkRec

    figure
    hold

    plot(clkRec(:,1), 'r');
    plot(clkRec(:,2), 'g');
    plot(clkRec(:,3), 'bx');
