function diag_plot(path , VAR , NList , out_path, in_device)
    def_device = 'png';
    aveFile = strcat(path , '\' , VAR , 'aveA.dat');
    stdFile = strcat(path , '\' , VAR , 'stdA.dat');
    ave = load(aveFile);
    std = load(stdFile);
    days = ave(:,3);
    
    
    for i = 1:max(size(NList)),
        figure(i)
        N = NList(i);
        errorbar(days , ave(:,N+3) , std(:,N + 3) , '-ro','LineWidth',2 , 'MarkerSize',10, 'MarkerEdgeColor','k', 'MarkerFaceColor','g')
        xlabel('Time (days)')
        title(sprintf('%s %d' , VAR , N));
        if nargin >= 4,
            if out_path == 1,
                out_path = path;
            end
            if out_path ~= 0,
                out_file = sprintf('%s\\%s-%d',out_path , VAR , N);
                if nargin == 4,
                    device = def_device;
                else
                    device = in_device;
                end
                print(i , strcat('-d',device) , out_file);
            end
        end
    end
end
        
    