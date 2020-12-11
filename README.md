# Komposter

Komposter is a lightweight music composing system intended mainly to be used in applications where the size of the executable must be minimized such as 4K and 64K intros.

It is built using a modular "virtual analog" model, where the composer can build the synthesizers from scratch using simple basic building blocks. This minimizes the amount of code required and relies more on data, which can be compressed more effectively.

A simple pattern-based sequencer is used to create songs which use up to 24 voices, each of which can use a different synthesizer. Each synthesizer can be programmed with a number of patches that can be switched between patterns.

Included with Komposter is a music player with full x86 assembly source code as well as a converter for generating nasm-includeable files from song files. Source code for the converter is also provided.

As Komposter is still in beta stage, please give me feedback on any bugs you may encounter or features you'd like to see added. Bugs and crashes are very likely, so please save your work often.


### Screenshots

![Screenshot](https://github.com/electronoora/komposter/blob/master/doc/images/k2.png)
![Screenshot](https://github.com/electronoora/komposter/blob/master/doc/images/k3.png)
![Screenshot](https://github.com/electronoora/komposter/blob/master/doc/images/k4.png)
![Screenshot](https://github.com/electronoora/komposter/blob/master/doc/images/k1.png)
![Screenshot](https://github.com/electronoora/komposter/blob/master/doc/images/k5.png)



### Building (macOS)

```
git clone https://github.com/electronoora/komposter.git
cd komposter
make -f Makefile.darwin dist
```

Using finder, navigate to the directory you cloned Komposter into and run
Komposter.app. Alternatively, you can open Komposter-yyyy-mm-dd.dmg and move
the app to your /Applications.



### Building (Linux, FreeBSD, etc)

Install the development packages for Freetype2, GLUT, MesaGL, OpenAL and
pkg-config. For example, on Ubuntu 16.04:

```
sudo apt-get install libfreetype6-dev freeglut3-dev mesa-common-dev libopenal-dev pkg-config
```

Any dependencies are automatically installed along with these packages.
After installation is complete, type:

```
git clone https://github.com/electronoora/komposter.git
cd komposter
./configure
make
make install
```



### Examples

Some audio clips and screenshots can be found at <a href="http://komposter.haxor.fi/">komposter.haxor.fi</a>.

