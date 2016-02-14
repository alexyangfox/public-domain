echo This script downloads (and configures)
echo  Binutils:	http://ftp.gnu.org/gnu/binutils/binutils-2.16.1.tar.gz
echo  GCC:		http://ftp.gnu.org/gnu/gcc/gcc-4.0.2/gcc-core-4.0.2.tar.bz2
echo  Bochs:	
echo leaving the system with a working GCC cross compiler and a debug build of the
echo Bochs emulator. This script should be run as root.
wget http://ftp.gnu.org/gnu/binutils/binutils-2.16.1.tar.gz -O /usr/src/binutils-2.16.1.tar.gz
wget http://ftp.gnu.org/gnu/gcc/gcc-4.0.2/gcc-core-4.0.2.tar.bz2 -O /usr/src/gcc-core-4.0.2.tar.bz2
cd /usr/src
gzip -d ./binutils-2.16.1.tar.gz
bzip2 -d ./gcc-core-4.0.2.tar.bz2
tar -xvf *.tar
mkdir build-binutils build-gcc
cd build-binutils
../binutils-2.16.1/configure --target=i586-elf --prefix=/usr/cross --disable-nls
make all install
cd ../build-gcc
export PATH=$PATH:/usr/cross/bin
../gcc-4.0.2/configure --target=i586-elf --prefix=/usr/cross --disable-nls --enable-languages=c --without-headers
ln -s /usr/cross/bin/* /usr/local/bin
wget http://kent.dl.sourceforge.net/sourceforge/bochs/bochs-2.2.6.tar.gz -O /usr/src/bochs-2.2.6.tar.gz
cd /usr/src
gzip -d ./bochs-2.2.6.tar.gz
tar -xvf bochs-2.2.6.tar
cd bochs-2.2.6
./configure --enable-debugger --enable-iodebug
make all install
