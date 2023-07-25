Flap character PCB generation
=============================
The PCB production files can be procedurally generated using [Inkscape](https://inkscape.org/) and [Autodesk Eagle](https://www.autodesk.com/products/eagle/free-download).

Requirements
------------
- You must have _Inkscape_ installed on your system.
- Install the _Inkscape_ extensions that are located in ```hardware/flaps/inkscape_scripts/```. 
- You must have _Autodesk Eagle_ installed on your system and it must be added to your PATH.
- The batch script to generate the production files only works on windows. So you will have to make your own if you are on linux.

Generate SVG's
--------------
1) Open the template svg ```flap_template.svg``` in inkscape.
2) Run the generate extention: ```>> Extentions >> Split Flap >> Flap Gen ...```.
3) Modify the generated characters or create custom characters.
2) Run the splitter extention: ```>> Extentions >> Split Flap >> Flap Split ...```. This can take some minutes as the extension has to open and close several new inkscape instances.
3) Place the generated ```.svg``` files in ```hardware/flaps/eagle_default/svg_files/```.

Convert the SVG's
-----------------
1) Open this [conversion utility](https://toonvaneyck.github.io/bulk-svgtoeagle/)
2) Upload all the generated ```.svg``` files and click convert. (Allow multiple downloads.)
3) Place the downloaded ```.scr``` files in ```hardware/flaps/eagle_default/scr_files/```.

Create production files
-----------------------

1) Run the ```scr_to_gerber.bat``` utility in the  ```hardware/flaps/eagle_default/``` directory. _Eagle_ will open and close a couple of times and ```.zip``` files containing the gerber files will be generated for all the flaps!
2) Upload to your favorite PCB manufacturer. 