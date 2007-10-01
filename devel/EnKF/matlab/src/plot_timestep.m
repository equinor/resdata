function plot_timestep(fignr , tstep , show_legend)
  data_size = max(tstep.forecast_size , tstep.analyzed_size);
  if tstep.use_true_time == 1
     x_0 = tstep.true_time * ones(data_size);
  else
     x_0 = tstep.time_step * ones(data_size);
  end
  
  if tstep.forecast_size > 0
     if tstep.analyzed_size > 0
        x_f = x_0 - 0.05;
        x_a = x_0 + 0.05;
     else
        x_f = x_0;
     end
  else
     x_a = x_0;
  end

  if tstep.forecast_size > 0
     plot(x_f , tstep.forecast_data , 'bo');
     hold on		
  end

  if tstep.analyzed_size > 0
     plot(x_a , tstep.analyzed_data , 'ro');
     hold on		
  end

  if show_legend == 1
     if tstep.forecast_size * tstep.analyzed_size > 0
        legend('Forecast','Analyzed');
     elseif tstep.forecast_size > 0
        legend('Forecast');
     elseif tstep.analyzed_size  > 0
        legend('Analyzed');
     end
  end	

