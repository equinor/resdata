struct enkf_config_struct {
  int  		    ens_size;
  hash_type        *config_hash;
  hash_type        *obs_hash;
  bool              endian_swap;
  path_fmt_type    * run_path;
  path_fmt_type    * ens_path_parameter;
  path_fmt_type    * ens_path_static;
  path_fmt_type    * ens_path_dynamic_forecast;
  path_fmt_type    * ens_path_dynamic_analyzed;
  int               Nwells;
  char            **well_list;
};
