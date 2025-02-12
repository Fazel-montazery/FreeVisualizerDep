# FreeVisualizer
**A cute, tiny, bloat-free music visualizer for average music enjoyer**
## Dependencies
debian-based distros:
```bash
sudo apt-get install -y mpg123 libsdl3-dev
```
fedora:
```bash
sudo dnf install -y mpg123 SDL3-devel
```
arch:
```bash
sudo pacman -S mpg123 sdl3
```
## Build
for building from source:
```bash
git clone https://github.com/Fazel-montazery/FreeVisualizer.git
cd FreeVisualizer
make
sudo make install # installing FreeVisualizer
```
you can change the default install directory (/usr/local):
```bash
sudo make INSDIR=/path/to/install install
```
if you wanna uninstall:
```bash
sudo make uninstall
```
## Usage
```bash
fv [OPTIONS] <mp3 file>
```
### Options
```
-h, --help				Print help
-s, --scene				Which scene(shader) to use
--ls                    list scenes
--yt                    Search and listen to music online (from youtube)
--lm                    list downloaded musics
```
