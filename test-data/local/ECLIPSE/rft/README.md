This is a case using RFT based on SPE1 (see license in SPE1.data) with output
created by opm flow 2023.04 . The .RFT file can be reproduced by [installing opm
flow](https://opm-project.org) and running

```bash
flow SPE1.DATA
```

In order to produce the .FRFT file, add 'FMTOUT' after
'UNIFOUT' in SPE1.DATA and change every occurrence of
'PROD' to 'FPROD' and every occurrence of 'INJ' to 'FINJ'.
