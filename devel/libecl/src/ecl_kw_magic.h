#ifndef __ECL_KW_MAGIC_H__
#define __ECL_KW_MAGIC_H__

#ifdef __cplusplus
extern "C" {
#endif


/*
  Some keyword strings which are give special significance when
  loading summary and restart files.
*/

#define INTEHEAD_KW  "INTEHEAD"    /* Restart files & init files*/
#define SEQNUM_KW    "SEQNUM"      /* Restart files */


#define SEQHDR_KW    "SEQHDR"      /* Summary files */
#define PARAMS_KW    "PARAMS"
#define MINISTEP_KW  "MINISTEP"
#define WGNAMES_KW   "WGNAMES"
#define KEYWORDS_KW  "KEYWORDS"
#define STARTDAT_KW  "STARTDAT"
#define UNITS_KW     "UNITS"
#define DIMENS_KW    "DIMENS"   // And grid files
#define NUMS_KW      "NUMS"
#define LGRS_KW      "LGRS"
#define NUMLX_KW     "NUMLX"
#define NUMLY_KW     "NUMLY"
#define NUMLZ_KW     "NUMLZ"

#define TIME_KW      "TIME"  /* RFT file block */


/* The INTEHEAD_XXX_INDEX values apply to both INIT files and restart files. */
#define INTEHEAD_DAY_INDEX     64   
#define INTEHEAD_MONTH_INDEX   65
#define INTEHEAD_YEAR_INDEX    66
#define INTEHEAD_VERSION_INDEX 94    /* This is ECLIPSE100 || ECLIPSE300 - not temporal version. */
#define INTEHEAD_PHASE_INDEX   14



#define LGR_KW         "LGR"
#define LGR_PARENT_KW  "LGRPARNT"
#define GLOBAL_STRING  "GLOBAL"
#define GRIDHEAD_KW    "GRIDHEAD"
#define ZCORN_KW       "ZCORN"
#define COORD_KW       "COORD"     // GRID
#define ACTNUM_KW      "ACTNUM"
#define MAPAXES_KW     "MAPAXES"
#define HOSTNUM_KW     "HOSTNUM"
#define COORDS_KW      "COORDS"    // EGRID
#define CORNERS_KW     "CORNERS"    




#ifdef __cplusplus
}
#endif
#endif
