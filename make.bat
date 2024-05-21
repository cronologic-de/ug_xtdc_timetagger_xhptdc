@ECHO OFF

pushd %~dp0

REM Command file for building latex documentation

if "%1" == "tt4" (
    set FILE=TimeTagger4_User_Guide.tex
) else if "%1" == "xtdc4" (
    set FILE=xTDC4_User_Guide.tex
) else if "%1" == "xhptdc8" (
    set FILE=xHPTDC8_User_Guide.tex
) else if "%1" == "all" (
    latexmk -lualatex TimeTagger4_User_Guide.tex
    latexmk -lualatex xTDC4_User_Guide.tex
    latexmk -lualatex xHPTDC8_User_Guide.tex
    goto end
) else if "%1" == "clean" (
    latexmk -c
    del *.atfi 2>nul
    goto end
) else goto help

latexmk -lualatex %FILE%
goto end

:help
echo.Invalid call signature.
echo.Call like '.\make.bat <option>', e.g. '.\make.bat tt4'
echo.
echo.Options:
echo.tt4
echo.  Build TimeTagger4 User Guide
echo.
echo.xtdc4
echo.  Build xTDC4 User Guide
echo.
echo.xhptdc8
echo.  Build xHPTDC8 User Guide
echo.
echo.all
echo.  Build all User Guides
echo.
echo.clean
echo.  Delete LaTeX intermediate files.

:end
popd