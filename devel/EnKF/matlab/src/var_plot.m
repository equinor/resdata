function var_plot(days , data , T , fignr , out_path , in_device)
  figure(fignr)
  plot(days , data(:,1) , 'o' ,  'MarkerSize',7, 'MarkerEdgeColor','k', 'MarkerFaceColor','k')
  hold on
  errorbar(days , data(:,2) , 3*data(:,3) , '-ro' , 'Color' ,'r', 'MarkerSize',5, 'MarkerEdgeColor','k', 'MarkerFaceColor','g')
  datetick('x',1);	    
  title(T)
  hold off
%%  if out_path ~= 0,
%%      out_file = sprintf('%s%s%s-%d',out_path , sep , VAR , N);
%%      if nargin == 7,
%%          device = def_device;
%%      else
%%          device = in_device;
%%      end
%%      print(i , strcat('-d',device) , out_file);
%%  end
