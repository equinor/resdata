function hist_plot(tstep)
  [f  , xf] = hist(tstep.forecast_data , sqrt(tstep.forecast_size));
  [a  , xa] = hist(tstep.analyzed_data , sqrt(tstep.analyzed_size));
  w = 0.75;
  bar(xf,f,w,'b');
  hold on
  bar(xa,a,w,'r');
  hold off
