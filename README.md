# play.c
play.c dead simple tui music player writrn in c

## how to ues:
###comand line flags:
- `-r` play in random order
- `-l` play in loop

### keys
- `q`     or `C-c` quit
- `p`     previus song
- `n`     next song
- `space` stop/play
- `r`     play in random order
- `l`     play in loop


## how to build:
### arch
```sh
sudo pacman -S mpg123 libao
make
sudo make install
```

### ubuntu
```sh
sudo apt install libmpg123-dev libao-dev
make
sudo make install
```
