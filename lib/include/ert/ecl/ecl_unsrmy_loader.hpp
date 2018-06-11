#include <string>
#include <vector>
#include <map>

#include <ert/ecl/ecl_file.hpp>

namespace ecl {

class unsmry_loader {

  int size;  //Number of entries in the smspec index
  ecl_file_type * file;
  
  std::map<int, std::vector<float>> cache;

  void read_data(int pos);

  public:
    unsmry_loader(int size_, const std::string& filename_);
    ~unsmry_loader();

    const std::vector<float>& get_vector(int pos);
};







}
