function [data] = ins_plot(filename)
  [data , plot_title] = fread_thist(filename);
  ins_plot__(data , plot_title , 1);
