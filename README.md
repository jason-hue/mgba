# mGBA Minimal SDL

This is a minimal SDL2-based implementation of mGBA.

## Building

1.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```

2.  **Configure the project:**
    ```bash
    cmake ..
    ```

3.  **Build the SDL target:**
    ```bash
    make -j$(nproc)
    ```

## Running

Launch the emulator by providing the path to a GBA ROM file:

```bash
./mgba-sdl path/to/your/game.gba
```

**Example:**
```bash
./mgba-sdl ../roms/dragonball.gba
```

## Key Mappings

| Keyboard Key   | GBA Button |
|:---------------|:-----------|
| **X**          | A          |
| **Z**          | B          |
| **Enter**      | START      |
| **Backspace**  | SELECT     |
| **Arrow Keys** | D-Pad      |
| **A**          | L          |
| **S**          | R          |
