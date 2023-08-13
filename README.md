# chrmkr
## a command line tool to convert .chr files
this program allows you to convert chr files to/from common image formats (png, tga, bmp)

## usage
- `./chrmkr`, show help
- `./chrmkr input.* output.chr`, converts input into output.chr
- `./chrmkr input.chr output.*`, converts input into output.* (where * is png, tga or bmp)

## building
- libpng is currently the only dependency
- ansi c
- gnu make
- gcc, although should work with tcc
- `make`
- `make run`
- `make clean`
- `make test`, converts ako.chr into each of the supported image formats, views each one, and then converts back

## misc
- makefile uses [feh](https://feh.finalrewind.org/) for image viewing, [nasu](https://git.sr.ht/~rabbits/nasu) for chr viewing
- ako.chr is copyright of Devine Lu Linvega
