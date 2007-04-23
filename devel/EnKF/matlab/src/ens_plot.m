function ens_plot(prior_path , posterior_path , well_list , var_list , unit_list , out_path , in_device)
% The ens_plot() function is designed to plot the results of ensemble
% experiments. Before calling ens_plot() you must use the EnKF 11-e
% option to generate ensemble plot files (say 'n' to Tecplot header).
% 
%  prior_path    : Path to plot files from the prior ensemble.
% 
%  posterior_path: Path to plot files from the posterior ensemble.
% 
%  well_list     : A list of the wells you want to consider, the format 
%                  for this variable is: {'Well1', 'Well2'}.
% 
%  var_list      : A list of variables you want to consider, {'WGOR','WOPR'}.
% 
%  unit_list     : The units corresponding to the variables in var_list.
% 
% The variables out_path and in_device are for saving to file, they are optional, see
% the documentation of diag_plot() for details.


   def_device = 'png';
   lw        = 1.00;
   psize     = 2.00;
   nwell     = max(size(well_list));
   nvar      = max(size(var_list));
   sep       = filesep;
   
   fig_nr = 0;
   for iw=1:nwell,
       for ivar = 1:nvar,
           well = char(well_list(iw));
           var  = char(var_list(ivar));
           unit = char(unit_list(ivar));
           fig_nr = fig_nr + 1;
           prior_file     = strcat(prior_path , sep , well , '.' , var);
           posterior_file = strcat(posterior_path , sep , well , '.' , var);
           prior     = load(prior_file);
           posterior = load(posterior_file);
           prior_size     = size(prior , 2) - 2;
           posterior_size = size(posterior , 2) - 2;
   
           figure(fig_nr)
           grid 
           plot(prior(:,1) , prior(:,2) , 'ko' , 'MarkerFaceColor','k', 'MarkerSize', psize);
           hold on
           plot(prior(:,1)     , prior(:,3)     , 'b' , 'LineWidth' , lw);
           plot(posterior(:,1) , posterior(:,3) , 'r' , 'LineWidth' , lw);
           legend('History', 'Prior' , 'Posterior')
           xlabel('Time (days)')
           ylabel(sprintf('%s (%s)',var,unit));
           title(sprintf('%s: %s',well,var));
   
           for i=2:prior_size,
               plot(prior(:,1) , prior(:,i+2) , 'b' , 'LineWidth' , lw)
           end
   
           for i=2:posterior_size,
               plot(posterior(:,1) , posterior(:,i+2) , 'r' , 'LineWidth' , lw)
           end
           plot(prior(:,1) , prior(:,2) , 'ko' , 'MarkerFaceColor','k', 'MarkerSize', psize);
           hold off
   
           
           if nargin >= 6,
               if out_path ~= 0,
                   out_file = sprintf('%s%s%s-%s',out_path , sep , well , var);
                   if nargin == 6,
                       device = def_device;
                   else
                       device = in_device;
                   end
                   print(fig_nr , strcat('-d',device) , out_file);
               end
           end
       end
   end
   
       
   
   
