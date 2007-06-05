function prod_plot(year,month,day , path , well , out_path, in_device)
%prod_plot(year , month , day , path , VAR , NList , out_path , device)
%
%The prod_plot() function can be used to plot time-series of production.
%
% o Prior to calling diag_plot() yous *must* use the EnKF program
%   to extract the mean and standard deviation of the ensemble. 
%   This is done by using menu option 11-p and then analyse the
%   ensaveA and ensstdA files. 
%
%   The diag_plot() function currently *only* considers the analyzed
%   files.
%
% o The function has eight arguments, only the six first are
%   mandatory:
%
%   year / month / day : The starting date of the simulation.
%
%   [path:] This is the path to where the diagnostica variable from
%           EnKF are stored. If you have run EnKF in the directory
%  
%                /path/to/EnKF_model/A
%            
%           you should enter '/path/to/EnKF_model/A/Diag' as first 
%           argument.
%
%   [well:] The name of the well
%
%   The two last arguments are for generating hard copies:
%
%   [out_path]: Path were hard copies are stored.
%
%   [device]: Type of files to plot, e.g. 'png', 'eps',..., defaults
%             to 'png'.

	      
    def_device = 'png';
    sep = filesep;
    dataFile = strcat(path , sep , well);
    data = load(dataFile);
    start_day = datenum(year,month,day);
    days = data(:,2) + start_day;
    OPR = data(:,3:5);
    GOR = data(:,6:8);
    WCT = data(:,9:11);
    if nargin == 5,
       out_path = 0;
    end
    device = def_device;
    if nargin == 7,
       device = in_device;
    end

    var_plot(days , OPR , sprintf('%s - OPR' , well) , 1 , out_path , device);
    var_plot(days , GOR , sprintf('%s - GOR' , well) , 2 , out_path , device);
    var_plot(days , WCT , sprintf('%s - WCT' , well) , 3 , out_path , device);
                
  

  
%&%    for i = 1:max(size(NList)),
%&%        figure(i)
%&%        N = NList(i);
%&%        errorbar(days , ave(:,N+3) , std(:,N + 3) , '-ro','LineWidth',2 , 'MarkerSize',10, 'MarkerEdgeColor','k', 'MarkerFaceColor','g')
%&%        xlabel('Time (days)')
%&%        title(sprintf('%s %d' , VAR , N));
%&%        if nargin >= 6,
%&%            if out_path == 1,
%&%                out_path = path;
%&%            end
%&%            if out_path ~= 0,
%&%                out_file = sprintf('%s%s%s-%d',out_path , sep , VAR , N);
%&%                if nargin == 7,
%&%                    device = def_device;
%&%                else
%&%                    device = in_device;
%&%                end
%&%                print(i , strcat('-d',device) , out_file);
%&%            end
%&%        end
%&%    end
%&%end
        
    
