#include <fortio.hpp>

static FortIO reader( const char * filename , bool endian_flip_header , bool fmt_file) {
  c_ptr = fortio_open_reader( filename , endian_flip_header , fmt_file );
}
