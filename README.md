# ug_xtdc4_timetagger
Userguides for the xTDC4 and TimeTagger4 time to digital converters

This is a LaTeX2e project that creates the userguides for the [xTDC4](https://www.cronologic.de/products/tdcs/xtdc4-pcie) and [TimeTagger4](https://www.cronologic.de/products/tdcs/timetagger) time to digital converters by cronologic GmbH & Co. KG.
The interface of the two devices is almost identical. Therefore both user guides are created from the same source. The user guide for the TimeTagger4 user guide is created when the macro `\deviceName` is set to `TimeTagger4`. Otherwise the user guide for the xTDC4 is created. You can control this from the command line like this:
```shell
pdflatex "\newcommand{\deviceName}{TimeTagger4} \input{xTDC4_TT_User_Guide.tex}"
```

## License
This documentation is licensed under an [Creative Commons Attribution-NoDerivatices 4.0 International License](https://creativecommons.org/licenses/by-nd/4.0/).
You are free to copy and redistribute the material in any medium or format for any purpose, even commercially unchanged if you give appropriate credit to cronologic GmbH & Co. KG. A link to [this repository](https://github.com/cronologic-de/ug_xhptdc8 ) or the product pages is sufficient.

If you decide to contribute to this repository you transfer non-exclusive but unlimited rights to your edit to cronologic GmbH & Co. KG.

![Creative Commons by-nd 4.0](https://i.creativecommons.org/l/by-nd/4.0/88x31.png)

The file [extraplaceins.sty](extraplaceins.sty) is in the public domain.
