# build mips, to set sysroot dir in msa.cmake
   mkdir build
   cd build
   cmake -DCMAKE_TOOLCHAIN_FILE=../msa.cmake ..

# transfor jpg to utf-8 data
   ./script/convertImage.py wordline.jpg

# put file in the board.
   adb push build/lstm_v2_tiny usr/demodir/
   adb push models/lite.data   usr/demodir/
   adb push wordline.jpg.data  usr/demodir/

# run demo
   ./lstm_v2_tiny -a lite.data -m wordline.jpg.data
