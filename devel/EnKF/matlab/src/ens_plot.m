function ens_plot(year,month,day,prior_path , posterior_path , well_list , var_list , unit_list , out_path , in_device)
% ens_plot(year , month , day , prior_path , posterior_path , well_list , var_list , unit_list)
%
% The ens_plot() function is designed to plot the results of ensemble
% experiments. Before calling ens_plot() you must use the EnKF 11-e
% option to generate ensemble plot files (say 'n' to Tecplot header).
%
%  year / month / day : The starting date of the simulation.
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

   if exist(prior_path,'dir') == 0
      disp(sprintf('Could not locate prior_path: %s - returning from ens_plot.' , prior_path));
      return;
   end

   if exist(posterior_path,'dir') == 0
      disp(sprintf('Could not locate posterior_path: %s - returning from ens_plot. ', posterior_path));
      return;
   end

   def_device = 'png';
   lw        = 1.00;
   psize     = 2.00;
   nwell     = max(size(well_list));
   nvar      = max(size(var_list));
   sep       = filesep;
   start_day = datenum(year,month,day);

   fig_nr = 0;
   for iw=1:nwell,
       for ivar = 1:nvar,
           well = char(well_list(iw));
           var  = char(var_list(ivar));
           unit = char(unit_list(ivar));
           fig_nr = fig_nr + 1;
           prior_file     = strcat(prior_path , sep , well , '.' , var);
           posterior_file = strcat(posterior_path , sep , well , '.' , var);
	   if exist(prior_file , 'file') == 0;
	      disp(sprintf('Could not find file: %s - returning from ens_plot',prior_file));
	      return;
	   end

	   if exist(posterior_file , 'file') == 0;
	      disp(sprintf('Could not find file: %s - returning from ens_plot',posterior_file));
	      return;
	   end

           prior     = load(prior_file);
           posterior = load(posterior_file);
           prior_size     = size(prior , 2) - 2;
           posterior_size = size(posterior , 2) - 2;
   
           figure(fig_nr)
	   plist = [];
           grid 
	   prior_dates     = prior(:,1) + start_day;
	   posterior_dates = posterior(:,1) + start_day;
	   plot(prior_dates , prior(:,2) , 'ko' , 'MarkerFaceColor','k', 'MarkerSize', psize);
	   

           hold on
           p = plot(prior_dates     , prior(:,3)     , 'b' , 'LineWidth' , lw); 
	   set(p , 'userdata' , 'Prior: 1');
	   plist = [plist , p];

           p = plot(posterior_dates , posterior(:,3) , 'r' , 'LineWidth' , lw);
	   set(p , 'userdata' , 'Posterior: 1');
	   plist = [plist , p];


           legend('History', 'Prior' , 'Posterior')
           xlabel('Time (days)')
           ylabel(sprintf('%s (%s)',var,unit));
           title(sprintf('%s: %s',well,var));
   
           for i=2:prior_size,
	      p = plot(prior_dates , prior(:,i+2) , 'b' , 'LineWidth' , lw) ; 
	      set(p , 'userdata' , sprintf('Prior: %d',i));
	      plist = [plist , p];
           end
   
           for i=2:posterior_size,
               p = plot(posterior_dates , posterior(:,i+2) , 'r' , 'LineWidth' , lw);
	       set(p , 'userdata' , sprintf('Posterior: %d',i));
	       plist = [plist , p];
           end
           plot(prior_dates , prior(:,2) , 'ko' , 'MarkerFaceColor','k', 'MarkerSize', psize);
           hold off


	   set(gcf,'windowbuttonupfcn','h=findobj(gca,''type'',''text'');delete(h)')
	   cb='pos=get(gca,''currentpoint'');h = text(pos(1,1),pos(1,2),get(gco,''userdata'')); set(h , ''FontSize'' , 12)';
	   set(plist , 'buttondownfcn',cb);   

           %%dayList = get(gca , 'XTick');
	   %%set(gca , 'XTickLabel' , {'Hei' , 'Joakim' , 'Hei2', 'Joakim2'});
           datetick('x',1);
 
           if nargin >= 9,
               if out_path ~= 0,
                   out_file = sprintf('%s%s%s-%s',out_path , sep , well , var);
                   if nargin == 9,
                       device = def_device;
                   else
                       device = in_device;
                   end
                   print(fig_nr , strcat('-d',device) , out_file);
               end
           end
       end
   end
   
       
   
   
