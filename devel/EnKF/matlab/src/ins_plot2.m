function [data] = ins_plot2(prefix)
  [data1 , title1] = fread_thist(strcat(prefix , '.enkf'));
  [data2 , title2] = fread_thist(strcat(prefix , '.eclipse'));

  ins_plot__(data1 , title1, 1);
  ins_plot__(data2 , title2, 2);
