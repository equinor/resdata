#ifndef ERT_SMSPEC_NODE_HPP
#define ERT_SMSPEC_NODE_HPP

#include <stdbool.h>
#include <stdio.h>

#include <string>
#include <array>

#include <ert/util/type_macros.hpp>

#include <resdata/smspec_node.h>

namespace rd {

class smspec_node {
private:
    std::string wgname;
    std::string
        keyword;      /* The value of the KEYWORDS vector for this elements. */
    std::string unit; /* The value of the UNITS vector for this elements. */
    int num; /* The value of the NUMS vector for this elements - NB this will have the value SMSPEC_NUMS_INVALID if the smspec file does not have a NUMS   vector. */
    std::string
        lgr_name; /* The lgr name of the current variable - will be NULL for non-lgr variables. */
    std::array<int, 3> lgr_ijk;

    /*------------------------------------------- All members below this line are *derived* quantities. */

    std::string
        gen_key1; /* The main composite key, i.e. WWCT:OP3 for this element. */
    std::string
        gen_key2; /* Some of the ijk based elements will have both a xxx:i,j,k and a xxx:num key. Some of the region_2_region elements will have both a xxx:num and a xxx:r2-r2 key. Mostly NULL. */
    rd_smspec_var_type var_type; /* The variable type */
    std::array<int, 3>
        ijk; /* The ijk coordinates (NB: OFFSET 1) corresponding to the nums value - will be NULL if not relevant. */
    bool
        rate_variable; /* Is this a rate variable (i.e. WOPR) or a state variable (i.e. BPR). Relevant when doing time interpolation. */
    bool total_variable; /* Is this a total variable like WOPT? */
    bool historical;     /* Does the name end with 'H'? */
    int params_index; /* The index of this variable (applies to all the vectors - in particular the PARAMS vectors of the summary files *.Snnnn / *.UNSMRY ). */
    float default_value; /* Default value for this variable. */

    static rd_smspec_var_type identify_special_var(const char *var);
    void set_wgname(const char *wgname);
    void set_num(const int grid_dims[3], int num_);
    void set_gen_keys(const char *key_join_string_);
    void decode_R1R2(int *r1, int *r2) const;
    void set_lgr_ijk(int lgr_i, int lgr_j, int lgr_k);

public:
    static rd_smspec_var_type valid_type(const char *keyword,
                                         const char *wgname, int num);
    int cmp(const smspec_node &node2) const;
    static int cmp(const smspec_node &node1, const smspec_node &node2);

    smspec_node(int param_index, const char *keyword, const char *wgname,
                int num, const char *unit, const int grid_dims[3],
                float default_value, const char *key_join_string);

    smspec_node(int param_index, const char *keyword, const char *wgname,
                const char *unit, const char *lgr, int lgr_i, int lgr_j,
                int lgr_k, float default_value, const char *key_join_string);

    smspec_node(int param_index, const char *keyword, const char *unit,
                float default_value);
    smspec_node(int param_index, const char *keyword, int num, const char *unit,
                const int grid_dims[3], float default_value,
                const char *key_join_string);
    smspec_node(int param_index, const char *keyword, int num, const char *unit,
                float default_value, const char *key_join_string);
    smspec_node(int param_index, const char *keyword, const char *wgname,
                const char *unit, float default_value,
                const char *key_join_string);
    smspec_node(int param_index, const char *keyword, const char *wgname,
                int num, const char *unit, float default_value,
                const char *key_join_string);
    smspec_node(const smspec_node &node, int param_index);

    static rd_smspec_var_type identify_var_type(const char *var);

    static int cmp(const smspec_node *node1, const smspec_node *node2) {
        return node1->cmp(*node2);
    }

    int get_R1() const;
    int get_R2() const;
    const char *get_gen_key1() const;
    const char *get_gen_key2() const;
    rd_smspec_var_type get_var_type() const;
    int get_num() const;
    const char *get_wgname() const;
    const char *get_keyword() const;
    const char *get_unit() const;
    bool is_rate() const;
    bool is_total() const;
    bool is_historical() const;
    bool need_nums() const;
    void fprintf__(FILE *stream) const;
    int get_params_index() const;
    float get_default() const;
    const std::array<int, 3> &get_ijk() const;
    const char *get_lgr_name() const;
    const std::array<int, 3> &get_lgr_ijk() const;
};

} // namespace rd

#endif
