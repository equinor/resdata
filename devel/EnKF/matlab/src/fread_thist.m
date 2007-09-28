function [data , plot_title]  = fread_thist(filename)
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
