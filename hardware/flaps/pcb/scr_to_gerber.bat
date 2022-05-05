set input_directory=scr_files
set output_directory=production_files
set tmp_directory=tmp

xcopy /YS %input_directory% %tmp_directory%\

for /f "delims=|" %%f in ('dir /b %tmp_directory%\') do (
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
    
    tar -a -cf %output_directory%\%%~nf.zip %tmp_directory%\%%~nf_gerber.*
)
RD /S /Q %tmp_directory% 