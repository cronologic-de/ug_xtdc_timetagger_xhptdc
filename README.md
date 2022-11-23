# ug_xtdc4_timetagger
Userguides for the xTDC4 and TimeTagger4 time to digital converters

This is a LaTeX2e project that creates the userguides for the [xHPTDC8](https://www.cronologic.de/products/tdcs/xhptdc8-pcie), [xTDC4](https://www.cronologic.de/products/tdcs/xtdc4-pcie) and [TimeTagger4](https://www.cronologic.de/products/tdcs/timetagger) time-to-digital converters by cronologic GmbH & Co. KG.
The interface of the two devices is almost identical. Therefore both user guides are created from the same source. Which user guide is created is controlled by the LaTeX macro `\deviceName`. It can be set to `xHPTDC8`, `xTDC4` or `TimeTagger4`. There are three top level files that control the configuration for the three users guides. 

To compile run
```shell
pdflatex xHPTDC8_User_Guide.tex
pdflatex xTDC4_User_Guide.tex
pdflatex TimeTagger4_User_Guide.tex
```
To update the xHPTDC8 source code example to the latest version from its [repository](https://github.com/cronologic-de/xhptdc8_babel/tree/main/ug_example), you should execute the following commands before compiling:
```shell
git fetch https://github.com/cronologic-de/xhptdc8_babel.git
git checkout FETCH_HEAD -- ug_example/ug_example_msvscpp/ug_example_msvscpp/xhptdc8_user_guide_example.cpp
mv .\ug_example\ug_example_msvscpp\ug_example_msvscpp\xhptdc8_user_guide_example.cpp .\xhptdc\example.cpp
git restore --staged .\ug_example\ug_example_msvscpp\ug_example_msvscpp\xhptdc8_user_guide_example.cpp
```

## License
This documentation is licensed under an [Creative Commons Attribution-NoDerivatices 4.0 International License](https://creativecommons.org/licenses/by-nd/4.0/).
You are free to copy and redistribute the material in any medium or format for any purpose, even commercially unchanged if you give appropriate credit to cronologic GmbH & Co. KG. A link to [this repository](https://github.com/cronologic-de/ug_xhptdc8 ) or the [product pages](https://www.cronologic.de/products/products-overview#tdcdata) is sufficient.

If you decide to contribute to this repository you transfer non-exclusive but unlimited rights to your edit to cronologic GmbH & Co. KG.

![Creative Commons by-nd 4.0](https://i.creativecommons.org/l/by-nd/4.0/88x31.png)

The file [extraplaceins.sty](extraplaceins.sty) is in the public domain.
