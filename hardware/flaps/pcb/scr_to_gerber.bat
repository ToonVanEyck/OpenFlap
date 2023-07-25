set input_directory=scr_files
set output_directory=production_files
set tmp_directory=tmp

rd /S /Q %output_directory%
md %output_directory% 
xcopy /YS %input_directory% %tmp_directory%\

for /f "delims=|" %%f in ('dir /b %tmp_directory%\') do (
    echo GRID mm;>>"%tmp_directory%\%%f"
    @REM echo CHANGE WIDTH 0.001 mm;>>"%tmp_directory%\%%f"
    echo LAYER top;>>"%tmp_directory%\%%f" 
    echo LINE 0 ^(0.4 2^) ^(0.4 2.2^)>>"%tmp_directory%\%%f" 
    echo LAYER bot;>>"%tmp_directory%\%%f" 
    echo LINE 0 ^(0.4 2^) ^(0.4 2.2^)>>"%tmp_directory%\%%f" 
    echo LAYER tStop;>>"%tmp_directory%\%%f" 
    echo LINE 0 ^(0.4 2^) ^(0.4 2.2^)>>"%tmp_directory%\%%f" 
    echo LAYER bStop;>>"%tmp_directory%\%%f" 
    echo LINE 0 ^(0.4 2^) ^(0.4 2.2^)>>"%tmp_directory%\%%f" 
    echo LAYER document;>>"%tmp_directory%\%%f" 
    echo CHANGE ALIGN TOP LEFT; TEXT Note: Soldermask on BOTH sides of PCB.\nDecoratitve PCB: Silkscreen and Soldermask Only.\nPCB Thickness : 0.8mm\nRemove Order Number : YES ^(0 -1^)>>"%tmp_directory%\%%f"
    echo WRITE %tmp_directory%\%%~nf.brd;>>"%tmp_directory%\%%f" 
    echo QUIT;>>"%tmp_directory%\%%f" 
    eaglecon.exe -S %tmp_directory%\%%f %tmp_directory%\%%~nf.brd
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_gerber.GKO %tmp_directory%\%%~nf.brd dimension
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_gerber.GBO %tmp_directory%\%%~nf.brd bPlace
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_gerber.GTO %tmp_directory%\%%~nf.brd tPlace
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_gerber.GBL %tmp_directory%\%%~nf.brd bot
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_gerber.GTL %tmp_directory%\%%~nf.brd top
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_gerber.GBS %tmp_directory%\%%~nf.brd bStop
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_gerber.GTS %tmp_directory%\%%~nf.brd tStop
    eaglecon.exe -X -d GERBER_RS274X -o %tmp_directory%\%%~nf_FabricationNotes.gbr %tmp_directory%\%%~nf.brd document
    tar -a -cf %output_directory%\%%~nf.zip %tmp_directory%\%%~nf_gerber.* %tmp_directory%\%%~nf_FabricationNotes.gbr
)
rd /S /Q %tmp_directory% 