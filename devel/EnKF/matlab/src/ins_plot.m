function [data] = ins_plot(filename)
  fid   = fopen(filename,'rb');
  plot_title = fread_string(fid);
  time_length = fread(fid , 1 , 'int');

  data = [];
  for itime = 1:time_length
     [active , tstep] = fread_timestep(fid);
     if active == 1
        data = [data; tstep];
     end
  end
  fclose(fid);


  figure(1)
  for i = 1:max(size(data))
     plot_timestep(1 , data(i));
  end 
  title(plot_title);
  xlabel('Time');
  hold off
