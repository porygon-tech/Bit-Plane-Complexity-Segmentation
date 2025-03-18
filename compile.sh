gcc -O2 -o embed embed.c -ljpeg -lpng
gcc -O2 -o extract extract.c -ljpeg -lpng
gcc -O2 -o extract_gui extract_gui.c $(pkg-config --cflags --libs gtk+-3.0) -ljpeg -lpng
gcc -O2 -o embed_gui embed_gui.c $(pkg-config --cflags --libs gtk+-3.0) -ljpeg -lpng
