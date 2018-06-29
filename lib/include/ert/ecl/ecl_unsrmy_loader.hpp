#include <string>
#include <vector>
#include <map>

#include <ert/ecl/ecl_file.hpp>

namespace ecl {

class unsmry_loader {

  int size;   //Number of entries in the smspec index
  int length; //Number of PARAMS in the UNSMRY file
  ecl_file_type      * file;
  ecl_file_view_type * file_view;
  
  std::map<int, std::vector<float>> cache;

  void read_data(int pos);

  public:
    unsmry_loader(int size_, const std::string& filename_);
    ~unsmry_loader();

    const std::vector<float>& get_vector(int pos);
};







}
