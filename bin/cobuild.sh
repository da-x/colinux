#!/bin/sh
# Thomas Fritzsche <tf@noto.de>, 2004 (c)
# 


function download()
{
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
  download http://ftp.debian.org/debian/pool/main/b/binutils \
     binutils_2.14.90.0.7.orig.tar.gz \
     3211f9065fd85f5f726f08c2f0c3db0c 
  download http://ftp.debian.org/debian/pool/main/b/binutils \
     binutils_2.14.90.0.7-5.diff.gz \
     3f1bf28faa4e82a97fc92d4f1508bf44                                                        
# gcc-core + gcc-g++ + patch
  download ftp://ftp.gwdg.de/pub/misc/gcc/releases/gcc-3.3.2 \
     gcc-core-3.3.2.tar.bz2 \
     42d1314aaa11a40e4c7c9fcb0f49259a
  download ftp://ftp.gwdg.de/pub/misc/gcc/releases/gcc-3.3.2 \
     gcc-g++-3.3.2.tar.gz \
     310cfb09876935692f949eef79d3c594
  download http://www.noto.de/colinux \
     gcc-3.3.2.diff \
     3efa14af3b00d511c6cc44dbc5fe9b49
# cygwin - used in gcc build process
  download http://ftp.gwdg.de/pub/linux/sources.redhat.com/cygwin/release/cygwin \
     cygwin-1.5.7-1.tar.bz2 \
     baca557381e4c11391aa71e5ed341c1d
# w32api + colinux ddk patch
  download http://heanet.dl.sourceforge.net/sourceforge/mingw \
     w32api-2.4-src.tar.gz \
     931b25da6223bd72ada13e83443cc6ed
  download http://www.noto.de/colinux/ \
     w32api.diff \
     bb914806d321b0bac831907e5c4ebab0
# Linux-Kernel 2.4.24
  download http://www.kernel.org/pub/linux/kernel/v2.4 \
     linux-2.4.24.tar.bz2 \
     1e055c42921b2396a559d84df4c3d9aa
# mini-xml
  download http://www.easysw.com/~mike/mxml \
     mxml-1.3.tar.gz \
     9b116daa370bf647447d6ffe70e73534
# fltk cross-gui-toolkit
  download http://heanet.dl.sourceforge.net/sourceforge/fltk \
     fltk-1.1.4-source.tar.bz2 \
     06ce1d3def2df35525592746faccbf98
  download http://www.noto.de/colinux/ \
     fltk1.diff \
      c6c74b7db501863ee134f54d84c55c67
#     b92aefbbb51f75e5f0d9ad82971de5b4
# coLinux-0.5.3-pre1 + network-patch
  download http://heanet.dl.sourceforge.net/sourceforge/colinux \
     colinux-20040210.tar.gz \
     f6f794a13d06c8b2c533363b9bab43de
#     66d055e5de1c8f8214cdc031e906b8dc
#  download http://www.noto.de/colinux/ \
#     colinux-0.5.3-pre1_1.diff \
#     77ae2e5e02c1363d8ab0ca4e72520867
}


function build_binutils()
{
  echo -n 'building binutils...'
  if test -f $CO_PREFIX/bin/i686-pc-cygwin-as ; then
    echo '[SKIP]'
    return 0
  fi
  cd $CO_BUILD
  tar xfz $CO_ROOT/binutils*.tar.gz
  cd binutils*
  zcat $CO_ROOT/binutils*.diff.gz | patch -p1 >> $CO_LOG 2>&1
  ./configure --target=i686-pc-cygwin --prefix=$CO_PREFIX >> $CO_LOG 2>&1
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
  tar xfj $CO_ROOT/gcc*core*.tar.bz2
  tar xfz $CO_ROOT/gcc-g++*.tar.gz
  cd gcc*
  patch -p1 < $CO_ROOT/gcc-3.3.2.diff >> $CO_LOG 2>&1
  ./configure --prefix=$CO_PREFIX --target=$CO_TARGET \
     --enable-languages=c,c++ >> $CO_LOG 2>&1
  make all PATH=$PATH:$CO_PREFIX/bin >> $CO_LOG 2>&1 
  make install PATH=$PATH:$CO_PREFIX/bin >> $CO_LOG 2>&1
  echo '[OK]'
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
  if test -f $CO_PREFIX/$CO_TARGET/lib/w32api/libwin32k.a ; then
    echo '[SKIP]'
    return 0
  fi
  cd $CO_BUILD
  tar xfz $CO_ROOT/w32api-*
  cd w32api*
  patch -p1 < $CO_ROOT/w32api.diff >> $CO_LOG 2>&1  
  ./configure --prefix=$CO_PREFIX --target=$CO_TARGET >> $CO_LOG 2>&1  
  make PATH=$PATH:$CO_PREFIX/bin \
       CC=$CO_TARGET-gcc \
       DLLTOOL=$CO_TARGET-dlltool \
       AS=$CO_TARGET-as \
       RANLIB=$CO_TARGET-ranlib \
       AR=$CO_TARGET-ar \
       LD=$CO_TARGET-ld \
       WINDRES=$CO_TARGET-windres >> $CO_LOG 2>&1  
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
 patch -p1 < $CO_ROOT/fltk1.diff >> $CO_LOG 2>&1
 make install PATH=$PATH:$CO_PREFIX/bin \
   prefix=$CO_PREFIX/$CO_TARGET >> $CO_LOG 2>&1
 cp -a FL $CO_PREFIX/$CO_TARGET/include
 echo '[OK]'
}

function build_colinux()
{
  echo -n 'building colinux'
  cd $CO_BUILD
  tar xfz $CO_ROOT/colinux-*tar.gz
  if ! test -f $CO_BUILD/linux/vmlinux ; then
    tar xfj $CO_ROOT/linux-*
    mv linux-* linux
    cd colinux*
    cp -av patch/linux $CO_BUILD/linux/colinux.diff
    cp -av conf/linux-config $CO_BUILD/linux/.config 
    cd $CO_BUILD/linux
    patch -p1 < colinux.diff
    make oldconfig
    make dep
    make vmlinux
  fi
  cd $CO_BUILD/colinux*
# patch -p1 < $CO_ROOT/colinux-0.5.3-pre1_1.diff
  cd $CO_BUILD/colinux*
  cd src
  make colinux PATH=$PATH:$CO_PREFIX/bin  
}


function build_all()
{
  CO_BUILD=$CO_ROOT/build 
  if ! test -f $CO_BUILD ; then
    mkdir -p $CO_BUILD
  fi
  build_binutils
 
  cd $CO_BUILD
  rm -rf cygwin*
  tar xfj $CO_ROOT/cygwin*
  if ! test -f $CO_PREFIX/$CO_TARGET ; then
    mkdir -p $CO_PREFIX/$CO_TARGET
  fi
  cp -a usr/* $CO_PREFIX/$CO_TARGET/
 
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
CO_TARGET=i686-pc-cygwin

if ! test -f $CO_PREFIX ; then
  mkdir -p $CO_PREFIX
fi

build_all 
cd $CO_WD