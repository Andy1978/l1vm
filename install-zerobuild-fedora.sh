#!/bin/bash
# changed: install to /home/foo/bin instead to /usr/local/bin!

echo "building compiler, assembler and VM..."

export CC=clang
export CCPP=clang++

sudo dnf install SDL2-devel.x86_64
sudo dnf install SDL2_gfx-devel.x86_64
sudo dnf install SDL2_image-devel.x86_64
sudo dnf install SDL2_ttf-devel.x86_64
sudo dnf install SDL2_mixer-devel.x86_64
sudo dnf install fann-devel.x86_64
sudo dnf install mpfr-devel.x86_64
sudo dnf install cmake.x86_64
sudo dnf install make.x86_64
sudo dnf install git.x86_64

# check if clang C compiler is installed
FILE=/usr/bin/clang
if test -f "$FILE"; then
    echo "$FILE exists!"
else
	sudo dnf install clang
fi

# check if ~/bin exists
DIR="~/bin"
if [ -d "$DIR" ]; then
  ### Take action if $DIR exists ###
  echo "${DIR} already exists!"
else
  ###  Control will jump here if $DIR does NOT exists ###
  echo "${DIR} will be created now..."
  mkdir ~/bin
fi

# check if zerobuild installed into ~/bin
FILE=~/bin/zerobuild
if test -f "$FILE"; then
    echo "$FILE exists!"
else
	echo "zerobuild not installed into $FILE!"
	echo "cloning and building it now..."
	git clone https://github.com/koder77/zerobuild.git
	cd zerobuild
	./make.sh
	cp zerobuild ~/bin/
	cd ..
fi

# check if mpreal.h is installed
FILE=/usr/local/include/mpreal.h
if test -f "$FILE"; then
    echo "$FILE exists!"
else
	echo "mpreal.h not installed into $FILE!"
	echo "cloning and building it now..."
	git clone https://github.com/advanpix/mpreal.git
	cd mpreal
	sudo cp mpreal.h /usr/local/include/
	cd ..
fi

cd assemb
if zerobuild force; then
	echo "l1asm build ok!"
else
	echo "l1asm build error!"
	exit 1
fi

cd ../comp
if zerobuild force; then
	echo "l1com build ok!"
else
	echo "l1com build error!"
	exit 1
fi

cd ../prepro
if zerobuild force; then
	echo "l1pre build ok!"
else
	echo "l1pre build error!"
	exit 1
fi

cd ../vm
if zerobuild zerobuild-nojit.txt force; then
	echo "l1vm JIT build ok!"
else
	echo "l1vm JIT build error!"
	exit 1
fi
cp l1vm l1vm-nojit
cd ..
cp assemb/l1asm ~/bin
cp comp/l1com ~/bin
cp prepro/l1pre ~/bin
cp vm/l1vm-jit ~/bin
echo "VM binaries installed into ~/bin"

cd modules
echo "installing modules..."
chmod +x *.sh
./build.sh
if ./install.sh; then
	echo "modules build ok!"
else
	echo "modules build FAILED!"
	exit 1
fi

echo "all modules installed. building programs..."
cd ../prog
chmod +x *.sh
if ./build-all.sh; then
	echo "building programs successfully!"
else
	echo "building programs FAILED!"
fi

echo "checking for ~/l1vm directory..."

# check if ~/l1vm exists
DIR="~/l1vm"
if [ -d "$DIR" ]; then
  ### Take action if $DIR exists ###
  echo "${DIR} already exists!"
else
  ###  Control will jump here if $DIR does NOT exists ###
  echo "${DIR} will be created now..."
  mkdir ~/l1vm
fi

cd ..

echo "installing programs to ~/l1vm"
cp prog/ ~/l1vm -r
cp lib/sdl-lib* ~/l1vm/prog

echo "installing lib to ~/lib"
cp lib/ ~/l1vm -r

echo "installing fonts to ~/l1vm"
cp fonts/ ~/l1vm -r

mkdir ~/l1vm/include
cp include-lib/* ~/l1vm/include/

echo "installation finished!"
exit 0
