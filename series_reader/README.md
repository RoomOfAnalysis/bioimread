#

## series reader

- extract meta info and convert different formats image into seperated tiff files by [bioformats comlinetools](https://bio-formats.readthedocs.io/en/latest/users/comlinetools/)
  - extract meta info and save in xml format
    - ```cmd
      ./showinf -no-upgrade -nopix -novalid -omexml-only -cache "xxx.lsm" >  "meta.xml"
      ```
  - convert input image into seperate tiff files
    - ```cmd
      REM Note for Windows Users: The command interpreter for batch files needs the % characters to be doubled in order to process the sequencing variables correctly.
      ./bfconvert -overwrite -cache "xxx.lsm" "xxx_S%%sZ%%zC%%cT%%t.tiff"
      ```
- read xml to obtain meta info
- read single tiff file into memory per time
  - read a series of images with above filename naming convention (`xxx_S%%sZ%%zC%%cT%%t.tiff`)
  - [ ] implement buffered image reader
