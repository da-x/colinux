#!/bin/sh
# Thomas Fritzsche <tf@noto.de>, 2004 (c)
# 

function download()
{
# check wget 
if test "x`which wget`" = "x" ; then
	echo "You need to install wget."
	exit 1
fi

# download the file / skip if file exist
  if ! test -f $2 ; then
    wget -c $1/$2
  else
    echo -n '[SKIP Download]' 
  fi
# make a md5sum
  CO_SUM=`md5sum $2 | sed -e 's/[[:space:]][[:graph:]]*//g'`
  echo -n $2 $CO_SUM
  if test $3 = $CO_SUM ; then
    echo ' [OK]'
    return 0
  else
    echo ' [FAILED]'
    cd $CO_WD
    exit 1
  fi
}

function get_files()
{
# binutils
  download http://heanet.dl.sourceforge.net/sourceforge/mingw \
     binutils-2.15.90-20040222-1-src.tar.gz \
     2cc7e5ade6915364130a3da8ece1570e
# runtime libs+includes
  download http://heanet.dl.sourceforge.net/sourceforge/mingw \
     mingw-runtime-3.2.tar.gz  \
     ecfd49e08f20a88b7ba11a755f2b53c2
  download http://heanet.dl.sourceforge.net/sourceforge/mingw \
     w32api-2.5.tar.gz  \
     be74d8925d1e273336ecb0d9225867f1
# gcc-core + gcc-g++ + patch
  download http://heanet.dl.sourceforge.net/sourceforge/mingw \
     gcc-core-3.3.1-20030804-1-src.tar.gz \
     6b15c20707b83146d3c9294bd395a388
  download http://heanet.dl.sourceforge.net/sourceforge/mingw \
     gcc-g++-3.3.1-20030804-1-src.tar.gz \
     0286d4972fb37e36d9acc9dda440273b
# w32api + colinux ddk patch
  download http://heanet.dl.sourceforge.net/sourceforge/mingw \
     w32api-2.5-src.tar.gz \
     395369c2c0c67394e54855f7516de3d3 
  download http://www.noto.de/colinux/ \
      w32api-2.5.diff \
      be4e054eef7a451d75a4ab39c535bb3d 
  download http://www.noto.de/colinux \
     gcc-3.3.2.diff \
     3efa14af3b00d511c6cc44dbc5fe9b49
# Linux-Kernel 2.4.25
  download http://www.kernel.org/pub/linux/kernel/v2.4 \
     linux-2.4.26.tar.bz2 \
     88d7aefa03c92739cb70298a0b486e2c
# mini-xml
  download http://www.easysw.com/~mike/mxml \
     mxml-1.3.tar.gz \
     9b116daa370bf647447d6ffe70e73534
# fltk cross-gui-toolkit
  download http://heanet.dl.sourceforge.net/sourceforge/fltk \
     fltk-1.1.4-source.tar.bz2 \
     06ce1d3def2df35525592746faccbf98
  download http://www.noto.de/colinux/ \
     fltk-1.1.4-win32.diff \
     b8135e3e93d823b29576725f30d97492
   download http://winpcap.polito.it/install/bin \
     wpdpack_3_0.zip \
     a9efb895badaf0086fc5254068583332
  download http://www.colinux.org/snapshots \
     colinux-20040417.tar.gz \
     2aec9cafaba018f039f101e4a7c02064 
}


function build_binutils()
{
  echo -n 'building binutils...'
  if test -f $CO_PREFIX/bin/$CO_TARGET-as ; then
    echo '[SKIP]'
    return 0
  fi
  cd $CO_BUILD
  rm -rf binutils*
  tar xfz $CO_ROOT/binutils*.tar.gz
  mv binutils* binutils
  mkdir "binutils-$CO_TARGET"
  cd "binutils-$CO_TARGET"
  ../binutils/configure --target=$CO_TARGET --prefix=$CO_PREFIX >> $CO_LOG 2>&1
  make >> $CO_LOG 2>&1
  make install >> $CO_LOG 2>&1
  echo '[OK]'
}

function build_gcc()
{
  echo -n 'building  gcc...'
  if test -f $CO_PREFIX/bin/$CO_TARGET-gcc ; then
    echo '[SKIP]'
    return 0
  fi
  cd $CO_BUILD
  rm -rf gcc*
  tar xfz $CO_ROOT/gcc*core*.tar.gz
  tar xfz $CO_ROOT/gcc-g++*.tar.gz
  mv gcc* gcc
  mkdir "gcc-$CO_TARGET"
  cd "gcc-$CO_TARGET"
  ../gcc/configure -v --prefix=$CO_PREFIX --target=$CO_TARGET \
     	      --with-headers="$CO_PREFIX/$CO_TARGET/include" \
	      --with-gnu-as --with-gnu-ld \
              --without-newlib --disable-multilib >> $CO_LOG 2>&1
  make LANGUAGES="c c++" >> $CO_LOG 2>&1 
  make LANGUAGES="c c++" install >> $CO_LOG 2>&1
  echo '[OK]'
  exit 0
}

function build_mxml()
{
  echo -n 'building mxml...'
  if test -f $CO_PREFIX/$CO_TARGET/lib/libmxml.a ; then
     echo '[SKIP]'
     return 0
  fi
  cd $CO_BUILD
  tar xfz $CO_ROOT/mxml* 
  cd mxml*
  ./configure --prefix=$CO_PREFIX/$CO_TARGET >> $CO_LOG 2>&1  
  make PATH=$PATH:$CO_PREFIX/bin \
       CC=$CO_TARGET-gcc \
       DLLTOOL=$CO_TARGET-dlltool \
       AS=$CO_TARGET-as \
       RANLIB=$CO_TARGET-ranlib \
       AR=$CO_TARGET-ar \
       LD=$CO_TARGET-ld \
       TARGETS='libmxml.a mxmldoc' \
       WINDRES=$CO_TARGET-windres >> $CO_LOG 2>&1
 touch mxmldoc.1
 make install TARGETS=libmxml.a >> $CO_LOG 2>&1
 echo '[OK]'     
}

function build_w32api()
{
  echo -n 'building  w32api...'
#  if test -f $CO_PREFIX/$CO_TARGET/lib/w32api/libwin32k.a ; then
#    echo '[SKIP]'
#    return 0
#  fi
  cd $CO_BUILD
  tar xfz $CO_ROOT/w32api-*src.tar.gz
  cd w32api*
  patch -p1 < $CO_ROOT/w32api-2.5.diff >> $CO_LOG 2>&1  
  ./configure --prefix=$CO_PREFIX/$CO_TARGET --host=$CO_TARGET >> $CO_LOG 2>&1  
  make >> $CO_LOG 2>&1  
  make install >> $CO_LOG 2>&1  
  echo '[OK]'
}

function build_fltk()
{
 echo -n 'building  fltk...'
 if test -f $CO_PREFIX/$CO_TARGET/lib/libfltk.a ; then
    echo '[SKIP]'
    return 0
 fi
 cd $CO_BUILD
 tar xfj $CO_ROOT/fltk*source*bz2
 cd fltk*
 patch -p1 < $CO_ROOT/fltk-1.1.4-win32.diff >> $CO_LOG 2>&1
 ./configure --host=$CO_TARGET >> $CO_LOG 2>&1
 make -C src >> $CO_LOG 2>&1
 cp lib/*.a $CO_PREFIX/$CO_TARGET/lib
 cp -a FL $CO_PREFIX/$CO_TARGET/include
 echo '[OK]'
}

function build_colinux()
{
  echo -n 'building colinux...'
  cd $CO_BUILD
  tar xfz $CO_ROOT/colinux-*tar.gz >> $CO_LOG 2>&1
  if ! test -f $CO_BUILD/linux/vmlinux ; then
    tar xfj $CO_ROOT/linux-* >> $CO_LOG 2>&1
    mv linux-* linux
    cd colinux*
    cp -av patch/linux $CO_BUILD/linux/colinux.diff >> $CO_LOG 2>&1
    cp -av conf/linux-config $CO_BUILD/linux/.config >> $CO_LOG 2>&1 
    cd $CO_BUILD/linux
    patch -p1 < colinux.diff >> $CO_LOG 2>&1
    make oldconfig >> $CO_LOG 2>&1
    make dep >> $CO_LOG 2>&1
    make vmlinux >> $CO_LOG 2>&1
  fi
  cd $CO_BUILD/colinux*
  cd src
  make colinux PATH=$PATH:$CO_PREFIX/bin >> $CO_LOG 2>&1

# install pcap header **** what a hack

  cd $CO_BUILD
  unzip -f $CO_ROOT/wpdpack*.zip >> $CO_LOG 2>&1
  cp -av wpdpack/Lib/*.a $CO_PREFIX/$CO_TARGET/lib >> $CO_LOG 2>&1
  cp -av wpdpack/Include/* $CO_PREFIX/$CO_TARGET/include >> $CO_LOG 2>&1
  cd $CO_PREFIX/$CO_TARGET/include
  rm bittypes.h >> $CO_LOG 2>&1
  ln -s stdint.h bittypes.h >> $CO_LOG 2>&1
  mv NET net >> $CO_LOG 2>&1
  mv PCAP.H pcap.h >> $CO_LOG 2>&1
  mv ip6_misc.h IP6_misc.h >> $CO_LOG 2>&1

  cd $CO_BUILD/colinux* 
  cd src 

  make bridged_net_daemon PATH=$PATH:$CO_PREFIX/bin >> $CO_LOG 2>&1

  if ! test -f $CO_ROOT/bin ; then
    mkdir -p $CO_ROOT/bin
  fi
  cd ..
  cp -a ../linux/vmlinux  $CO_ROOT/bin
  cp -a src/colinux/os/winnt/user/conet-daemon/colinux-net-daemon.exe \
     $CO_ROOT/bin
  cp -a src/colinux/os/winnt/user/conet-bridged-daemon/colinux-bridged-net-daemon.exe \
     $CO_ROOT/bin 
  cp -a src/colinux/os/winnt/user/console/colinux-console-fltk.exe \
     $CO_ROOT/bin
  cp -a src/colinux/os/winnt/user/console-nt/colinux-console-nt.exe \
     $CO_ROOT/bin
  cp -a src/colinux/os/winnt/user/daemon/colinux-daemon.exe \
     $CO_ROOT/bin
  cp -a src/colinux/os/winnt/build/linux.sys \
     $CO_ROOT/bin
 echo '[OK]'
}


function build_all()
{
  CO_BUILD=$CO_ROOT/build 
  if ! test -f $CO_BUILD ; then
    mkdir -p $CO_BUILD
  fi
  build_binutils
 
  if ! test -f $CO_PREFIX/$CO_TARGET ; then
    mkdir -p $CO_PREFIX/$CO_TARGET
  fi

# install cross libs and includes
  cd $CO_PREFIX/$CO_TARGET
  tar xfz $CO_ROOT/mingw-runtime*.tar.gz
  tar xfz $CO_ROOT/w32api-2.5.tar.gz
  PATH="$CO_PREFIX/bin:$PATH"   
  build_gcc
  build_w32api
  build_mxml
  build_fltk
  build_colinux
}
CO_WD=`pwd`
cd `dirname $0`
CO_ROOT=`pwd`
cd $CO_ROOT
CO_LOG=$CO_ROOT/log
rm $CO_LOG >> $CO_LOG 2>&1
echo 'Building coLinux'
get_files

CO_PREFIX=$CO_ROOT/target
CO_TARGET=i686-pc-mingw32

if ! test -f $CO_PREFIX ; then
  mkdir -p $CO_PREFIX
fi

build_all 
cd $CO_WD
