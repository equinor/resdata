#include <ert/ecl/ecl_smspec.hpp>
#include <ert/ecl/Smspec.hpp>

namespace ecl {

    smspec_node::smspec_node( const smspec_node& rhs ) :
        node( smspec_node_alloc_copy( rhs.node.get() ) )
    {}

    smspec_node::smspec_node( smspec_node&& rhs ) :
        node( std::move( rhs.node ) )
    {}

    smspec_node& smspec_node::operator=( const smspec_node& rhs ) {
        this->node.reset( smspec_node_alloc_copy( rhs.node.get() ) );
        return *this;
    }

    smspec_node& smspec_node::operator=( smspec_node&& rhs ) {
        this->node = std::move( rhs.node );
        return *this;
    }

    int smspec_node::cmp( const smspec_node& node1, const smspec_node& node2) {
        return smspec_node_cmp( node1.get() , node2.get() );
    }

    static const int dummy_dims[ 3 ] = { -1, -1, -1 };
    const auto default_join = ":";

    static int global_index( const int dims[ 3 ], const int ijk[ 3 ] ) {
        /* num is offset 1 global index */
        return 1 + ijk[ 0 ] + ( ijk[ 1 ] * dims[ 0 ] ) + ( ijk[ 2 ] * dims[ 1 ] * dims[ 0 ] );
    }

    smspec_node::smspec_node(
            ecl_smspec_var_type var_type,
            const std::string& name,
            const std::string& kw
            ) : smspec_node( var_type, name.c_str(), kw.c_str(), "", default_join, dummy_dims, 0 )
    {}

    smspec_node::smspec_node( const std::string& keyword ) :
        smspec_node( ecl_smspec_identify_var_type( keyword.c_str() ),
                     "",
                     keyword.c_str(),
                     "",
                     default_join,
                     dummy_dims,
                     0 )
    {}

    smspec_node::smspec_node(
            const std::string& keyword,
            const int dims[ 3 ],
            const int ijk[ 3 ] ) :
        smspec_node(
            ECL_SMSPEC_BLOCK_VAR, "", keyword.c_str(), "", default_join, dims, global_index( dims, ijk )
        )
    {}

    smspec_node::smspec_node(
            const std::string& keyword,
            const std::string& wellname,
            const int dims[ 3 ],
            const int ijk[ 3 ] ) :
        smspec_node(
            ECL_SMSPEC_COMPLETION_VAR, wellname.c_str(), keyword.c_str(), "", default_join, dims, global_index( dims, ijk )
        )
    {}

    smspec_node::smspec_node(
            const std::string& keyword,
            const int dims[ 3 ],
            int region ) :
        smspec_node(
            ECL_SMSPEC_REGION_VAR, "", keyword.c_str(), "", default_join, dims, region
        )
    {}

    smspec_node::smspec_node(
            ecl_smspec_var_type type,
            const char* wgname,
            const char* keyword,
            const char* unit,
            const char* join,
            const int grid_dims[ 3 ],
            int num, int index, float default_value ) :
        node( smspec_node_alloc( type, wgname, keyword, unit,
                    join, grid_dims, num, index, default_value ) )
    {}

    smspec_node::smspec_node(ecl_smspec_var_type type,
                             const char* wgname,
                             const char* keyword,
                             const char* unit,
                             const char* lgr,
                             const char* join,
                             int lgr_i, int lgr_j, int lgr_k,
                             int index, float default_value ) :
        node( smspec_node_alloc_lgr( type, wgname, keyword, unit, lgr, 
                                     join, lgr_i, lgr_j, lgr_k, index, default_value ) )
    {}

    smspec_node * smspec_node_new(ecl_smspec_var_type var_type ,
                                  const char * wgname  ,
                                  const char * keyword ,
                                  const char * unit    ,
                                  const char * key_join_string ,
                                  const int grid_dims[3] ,
                                  int num , int param_index, float default_value)
    {
        return new smspec_node(var_type, wgname, keyword, unit, key_join_string, grid_dims, num, param_index, default_value);
    }

    smspec_node * smspec_node_new_lgr(ecl_smspec_var_type var_type ,
                                      const char * wgname  ,
                                      const char * keyword ,
                                      const char * unit    ,
                                      const char * lgr,
                                      const char * key_join_string ,
                                      int lgr_i, int lgr_j, int lgr_k,
                                      int param_index, float default_value)
    {
        return new smspec_node(var_type, wgname, keyword, unit, lgr, key_join_string, lgr_i, lgr_j, lgr_k, param_index, default_value);
    }

    void smspec_node_delete(smspec_node * ptr) {
        delete ptr;
    }

    int smspec_node::type() const {
        return smspec_node_get_var_type( this->node.get() );
    }

    const char* smspec_node::wgname() const {
        return smspec_node_get_wgname( this->node.get() );
    }

    const char* smspec_node::keyword() const {
        return smspec_node_get_keyword( this->node.get() );
    }

    const char* smspec_node::key1() const {
        return smspec_node_get_gen_key1( this->node.get() );
    }

    int smspec_node::num() const {
        return smspec_node_get_num( this->node.get() );
    }

    smspec_node_type* smspec_node::get() {
        return this->node.get();
    }

    const smspec_node_type* smspec_node::get() const {
        return this->node.get();
    }

    void smspec_node::set_params_index(int params_index) {
        smspec_node_set_params_index( this->node.get(), params_index );
    }


    int smspec_node::get_params_index() const {
        return smspec_node_get_params_index( this->node.get() );
    }


    int smspec_node::update_params_index(int params_index) {
        if (this->get_params_index() < 0)
            this->set_params_index(params_index);
        return this->get_params_index();
    }


    float smspec_node::get_default() const {
        return smspec_node_get_default(this->node.get() );
    }
}
