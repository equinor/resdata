#include <ecl_util.h>
#include <ecl_kw.h>

#include <fortio.hpp>


class EclKW {
private:
  ecl_kw_type * c_ptr;
  bool          owner;

  EclKW(ecl_kw_type * c_ptr , bool owner) {
    this->c_ptr = c_ptr;
    this->owner = owner;
  }
  
  
public:
  ecl_kw_type  * C_PTR( ) { return c_ptr; } 
    
  static EclKW create (const char * name ,  int size , ecl_type_enum type , void * data = NULL);
  static EclKW wrap   (ecl_kw_type * c_ptr , bool owner = false);
  static EclKW wrap_data(const char * name ,  int size , ecl_type_enum type, void * data);
  
  ~EclKW();
  void  fwrite( FortIO& fortio );

  int           size( ) { return ecl_kw_get_size( c_ptr ); }
  ecl_type_enum type()  { return ecl_kw_get_type( c_ptr ); }
  
  
  template <typename T> void set_data( const T * data ) {
    ecl_kw_set_memcpy_data( c_ptr , data );
  }

  template <typename T> void iset(int i , T value) { 
    ecl_kw_iset( c_ptr , i , &value);
  }
  
  template <typename T> T iget(int i ) {
    T value;
    ecl_kw_iget( c_ptr , i , &value);
    return value;
  }

};


