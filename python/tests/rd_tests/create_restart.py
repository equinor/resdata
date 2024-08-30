from resdata import ResDataType
from resdata.resfile import ResdataKW, openFortIO, FortIO


def create_restart(grid, case, p1, p2=None, rporv1=None, rporv2=None):
    with openFortIO(f"{case}.UNRST", mode=FortIO.WRITE_MODE) as f:
        seq_hdr = ResdataKW("SEQNUM", 1, ResDataType.RD_INT)
        seq_hdr[0] = 10
        p = ResdataKW("PRESSURE", grid.getNumActive(), ResDataType.RD_FLOAT)
        for i in range(len(p1)):
            p[i] = p1[i]

        header = ResdataKW("INTEHEAD", 67, ResDataType.RD_INT)
        header[64] = 1
        header[65] = 1
        header[66] = 2000

        seq_hdr.fwrite(f)
        header.fwrite(f)
        p.fwrite(f)

        if rporv1:
            rp = ResdataKW("RPORV", grid.getNumActive(), ResDataType.RD_FLOAT)
            for idx, val in enumerate(rporv1):
                rp[idx] = val

            rp.fwrite(f)

        if p2:
            seq_hdr[0] = 20
            header[66] = 2010
            for i in range(len(p2)):
                p[i] = p2[i]

            seq_hdr.fwrite(f)
            header.fwrite(f)
            p.fwrite(f)

        if rporv2:
            rp = ResdataKW("RPORV", grid.getNumActive(), ResDataType.RD_FLOAT)
            for idx, val in enumerate(rporv2):
                rp[idx] = val

            rp.fwrite(f)
