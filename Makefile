CC = gcc

DFV := $(shell command -v desktop-file-validate 2> /dev/null)
DFI := $(shell command -v desktop-file-install 2> /dev/null)
 
backlighter: backlighter.c
	$(CC) -o $@ $^
	chmod +x $@

p2013dim: p2013dim.c
	$(CC) -o $@ $^
	chmod +x $@

backlighter.desktop:
ifdef DFV
	desktop-file-validate $@
endif

all: p2013dim backlighter backlighter.desktop
	chmod +x pybacklight.py

install: all
	install -m 6711 p2013dim /usr/bin
	install -m 6711 backlighter /usr/bin
	install -m 6711 pybacklight.py /usr/bin
	install -m 0444 backlighter.png /usr/share/icons/hicolor/64x64
ifdef DFI
	desktop-file-install --dir=/usr/share/applications backlighter.desktop
endif