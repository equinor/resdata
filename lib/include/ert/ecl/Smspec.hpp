#ifndef OPM_ERT_SMSPEC_HPP
#define OPM_ERT_SMSPEC_HPP

#include <memory>
#include <string>

#include <ert/ecl/smspec_node.h>
#include <ert/util/ecl_unique_ptr.hpp>

namespace ecl {

    class smspec_node {
        public:
            smspec_node( smspec_node&& );
            smspec_node( const smspec_node& );
            explicit smspec_node( const std::string& keyword );

            smspec_node& operator=( const smspec_node& );
            smspec_node& operator=( smspec_node&& );

            smspec_node(ecl_smspec_var_type var_type,
                        const std::string& wgname,
                        const std::string& keyword);


            smspec_node( const std::string& keyword,
                    const int dims[ 3 ],
                    const int ijk[ 3 ] );

            smspec_node( const std::string& keyword,
                    const std::string& wellname,
                    const int dims[ 3 ],
                    const int ijk[ 3 ] );

            smspec_node( const std::string& keyword,
                    const int dims[ 3 ],
                    int region );

            static int cmp( const smspec_node& node1, const smspec_node& node2);
            int type() const;
            const char* keyword() const;
            const char* wgname() const;
            const char* key1() const;
            int num() const;
            smspec_node_type* get();
            const smspec_node_type* get() const;

            float get_default() const;

            void set_params_index(int params_index);
            int get_params_index( ) const;
            int update_params_index(int params_index);
        //  Should be private ...
            smspec_node(ecl_smspec_var_type var_type,
                        const char* wgname,
                        const char* keyword,
                        const char* unit,
                        const char* join,
                        const int[3],
                        int num,
                        int params_index = -1,
                        float default_value = 0);

            smspec_node(ecl_smspec_var_type var_type,
                        const char* wgname,
                        const char* keyword,
                        const char* unit,
                        const char* join,
                        const char* lgr_name,
                        int lgr_i,
                        int lgr_j,
                        int lgr_k,
                        int params_index = -1,
                        float default_value = 0);



    private:
      ecl::ecl_unique_ptr< smspec_node_type, smspec_node_free > node;
      //ecl::ecl_unique_ptr<smspec_node_type, nullptr> node;
      //std::unique_ptr<smspec_node_type> node;
    };

    smspec_node * smspec_node_new(ecl_smspec_var_type var_type ,
                                  const char * wgname  ,
                                  const char * keyword ,
                                  const char * unit    ,
                                  const char * key_join_string ,
                                  const int grid_dims[3] ,
                                  int num , int param_index, float default_value);


    smspec_node * smspec_node_new_lgr(ecl_smspec_var_type var_type ,
                                      const char * wgname  ,
                                      const char * keyword ,
                                      const char * unit    ,
                                      const char * lgr,
                                      const char * key_join_string ,
                                      int lgr_i, int lgr_j , int lgr_k,
                                      int param_index, float default_value);

    void smspec_node_delete(smspec_node * ptr);

}

namespace ERT {
    typedef ecl::smspec_node smspec_node;
}
#endif //OPM_ERT_SMSPEC_HPP
