# mGBA for Project-N AM/NEMU

This is a trimmed GBA-only mGBA porting project.

- Primary path: Project-N Abstract Machine / NEMU
- Secondary path: minimal SDL host frontend

## AM build and run

```bash
export AM_HOME=/home/knifefire/abstract-machine
export NEMU_HOME=/home/knifefire/nemu

make ARCH=native
make ARCH=native run
make ARCH=native run mainargs=dragonball

make ARCH=$ISA-nemu
make ARCH=$ISA-nemu run
make ARCH=$ISA-nemu run mainargs=dragonball
```

AM ROMs are embedded from `roms/*.gba`.

- `mainargs=<name>` selects an embedded ROM
- no `mainargs` uses the first embedded ROM

## SDL build and run

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
./mgba-sdl ../roms/dragonball.gba
```
## Upstream

- https://github.com/mgba-emu/mgba
- https://mgba.io/
