function rft_plot(path , well , out_path , in_device)
    def_device = 'png';
    true     = load(strcat(path,'\',well,'.T'));
    forecast = load(strcat(path,'\',well,'.F'));
    analyzed = load(strcat(path,'\',well,'.A'));
    fig_nr = 1;
    
    figure(fig_nr)
    plot(true(:,1),true(:,2),'ko','MarkerSize',4,'MarkerFaceColor','k')
    hold on
    plot(forecast(:,1)-0.25,forecast(:,6),'bo','MarkerSize',3,'MarkerFaceColor','b');
    plot(analyzed(:,1)+0.25,analyzed(:,6),'ro','MarkerSize',3,'MarkerFaceColor','r');
    legend('Measured value','Forecast ensemble','Analyzed ensemble')
    plot(true(:,1),true(:,2),'-k')
    plot(true(:,1),true(:,2),'ko','MarkerSize',6,'MarkerFaceColor','k')
    xlabel('Position along well (gridblocks)');
    ylabel('Pressure (bar)');
    title('RFT Ensemble plot')
    hold off
    if nargin >= 3,
        if out_path == 1,
            out_path = path;
        end
        if out_path ~= 0,
            out_file = sprintf('%s\\%s-%d',out_path , VAR , N);
            if nargin == 3,
                device = def_device;
            else
                device = in_device;
            end
            print(fig_nr , strcat('-d',device) , out_file);
        end
    end