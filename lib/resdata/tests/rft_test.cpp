#include <rd_rft_file.h>
#include <rd_rft_node.h>
#include <stringlist.h>
#include <util.h>

int main(int argc, char **argv) {
    if (argc != 2)
        util_exit("I want one RFT file - try again \n");
    {
        const char *filename = argv[1];
        rd_rft_file_type *rft_file = rd_rft_file_alloc(filename);
        stringlist_type *wells = rd_rft_file_alloc_well_list(rft_file);

        printf("<ECLIPSE RFT FILE>\n");
        {
            int iw;
            for (iw = 0; iw < stringlist_get_size(wells); iw++) {
                const char *well = stringlist_iget(wells, iw);
                printf("    <WELL>\n");
                {
                    int it;
                    for (it = 0;
                         it < rd_rft_file_get_well_occurences(rft_file, well);
                         it++) {
                        const rd_rft_node_type *node =
                            rd_rft_file_iget_well_rft(rft_file, well, it);
                        time_t date = rd_rft_node_get_date(node);
                        {
                            int mday, year, month;
                            util_set_date_values(date, &mday, &month, &year);
                            printf("        <RFT>\n");
                            printf("            <DATE>%02d/%02d/%4d</DATE> \n",
                                   mday, month, year);
                            {
                                int num_cells = rd_rft_node_get_size(node);
                                int icell;
                                for (icell = 0; icell < num_cells; icell++) {
                                    int i, j, k;
                                    rd_rft_node_iget_ijk(node, icell, &i, &j,
                                                         &k);
                                    printf("            <cell>\n");
                                    printf(
                                        "                <PRESSURE> %g "
                                        "</PRESSURE> \n",
                                        rd_rft_node_iget_pressure(node, icell));
                                    printf("                <DPETH>    %g "
                                           "</DEPTH>     \n",
                                           rd_rft_node_iget_depth(node, icell));
                                    printf("                <ijk> %3d,%3d,%3d "
                                           "</ijk>  \n",
                                           i, j, k);
                                    printf("            </cell>\n");
                                }
                            }
                            printf("        </RFT>\n");
                        }
                    }
                }
                printf("    </WELL>\n");
            }
        }
        printf("</ECLIPSE RFT FILE>\n");
        rd_rft_file_free(rft_file);
    }
}
