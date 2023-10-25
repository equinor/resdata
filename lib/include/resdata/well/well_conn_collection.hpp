#ifndef ERT_WELL_CONN_COLLECTION_H
#define ERT_WELL_CONN_COLLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ert/util/type_macros.hpp>

#include <resdata/rd_kw.hpp>

#include <resdata/well/well_conn.hpp>

typedef struct well_conn_collection_struct well_conn_collection_type;

well_conn_collection_type *well_conn_collection_alloc(void);
void well_conn_collection_free(well_conn_collection_type *wellcc);
void well_conn_collection_free__(void *arg);
int well_conn_collection_get_size(const well_conn_collection_type *wellcc);
const well_conn_type *
well_conn_collection_iget_const(const well_conn_collection_type *wellcc,
                                int index);
well_conn_type *
well_conn_collection_iget(const well_conn_collection_type *wellcc, int index);
void well_conn_collection_add(well_conn_collection_type *wellcc,
                              well_conn_type *conn);
void well_conn_collection_add_ref(well_conn_collection_type *wellcc,
                                  well_conn_type *conn);
int well_conn_collection_load_from_kw(well_conn_collection_type *wellcc,
                                      const rd_kw_type *iwel_kw,
                                      const rd_kw_type *icon_kw,
                                      const rd_kw_type *scon_kw,
                                      const rd_kw_type *xcon_kw, int iwell,
                                      const rd_rsthead_type *rst_head);

UTIL_IS_INSTANCE_HEADER(well_conn_collection);

#ifdef __cplusplus
}
#endif
#endif
