#include <string>
#include <iostream>

#include <ert/util/int_vector.hpp>

#include <ert/ecl/ecl_kw_magic.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_unsrmy_loader.hpp>


namespace ecl {

unsmry_loader::unsmry_loader(int size_, const std::string& filename_) :
  size(size_),
  file(ecl_file_open(filename_.c_str(), 0))
{
}


unsmry_loader::~unsmry_loader() {
  ecl_file_close(file);
}


const std::vector<float>& unsmry_loader::get_vector(int pos) {
  if (pos >= size)
     throw std::invalid_argument("unsmry_loader::get_vector: argument 'pos' mst be less than size of PARAMS.");
  if (cache.count(pos) == 0)
     //extracting data and inserting into cache
     read_data(pos);

  return cache[pos];
}


void unsmry_loader::read_data(int pos) {

   ecl_file_view_type * file_view = ecl_file_get_global_view( file );
   int data_size = ecl_file_view_get_num_named_kw( file_view , PARAMS_KW);
   cache[pos] = std::vector<float>( data_size );
   std::vector<float>& data = cache[pos];

   int_vector_type * index_map = int_vector_alloc( 1 , pos);
   char buffer[4];

   for (int index = 0; index < data_size; index++) {
      ecl_file_view_index_fload_kw(file_view, PARAMS_KW, index, index_map, buffer);
      float * data_value = (float*)buffer;
      data[index] = *data_value;
   }
   int_vector_free( index_map );
}

}
