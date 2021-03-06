TKDetection
===========

TKDetection is a software to detect and segment wood knots.

1. [Dependencies list](#dependencies-list)
2. [Dependencies installation on Ubuntu](#dependencies-installation-on-ubuntu)
3. [TKDetection installation](#tkdetection-installation)


Dependencies list
-----------------

|   |            Library                  |  Tested version  |   |          Library            |  Tested version  |   |        Library              |  Tested version  |
|:-:|:-----------------------------------:|:----------------:|---|:---------------------------:|:----------------:|---|:---------------------------:|:----------------:|
| 1 | [Qt](#1-qt)                         |       4.8        | 4 | [Qwt](#4-qwt)               |      6.0.2       | 7 | [DGtalTools](#7-dgtaltools) |       0.6        |
| 2 | [Armadillo](#2-armadillo)           |     3.800.2      | 5 | [Qxt](#5-qxt)               |      0.6.2       | 8 | [ImaGene](#8-imagene)       |      online      |
| 3 | [InsightToolkit](#3-insighttoolkit) |      4.3.1       | 6 | [QwtPolar](#6-qwtpolar)     |      1.0.1       | 9 | [KerUtils](#9-kerutils)     |      online      |



Dependencies installation on Ubuntu
-----------------------------------

To install all dependencies on Ubuntu 12.10 64 bits, you can tu use the shell script script_install_dependencies_ubuntu12.10_x86_64.sh available on the TKDetection directory.
We recomand to copy the script on a dedicated directory before to execute the following commands :

~~~
	chmod u+x script_install_dependencies_ubuntu12.10_x86_64.sh
	./script_install_dependencies_ubuntu12.10_x86_64.sh
~~~

When installation finished, you can remove the dedicated directory.


On another plateform, you must apply the following steps.

### Preliminaries

Install "build-essential" and "cmake" packages.


~~~
  sudo apt-get install build-essential cmake
~~~


### 1. QT
[Top](#tkdetection)

Install the *qtcreator* package available on the *Universe* repository.
This meta-package install the set of Qt dependencies.

~~~
  sudo apt-get install qtcreator
~~~


### 2. Armadillo
[Top](#tkdetection)

Download the last version available on the website http://arma.sourceforge.net.

It is recommended to install the libraries LAPACK, BLAS, ATLAS, and Boost to improve the performances, in particular for the matrix sum and product. 


~~~
  sudo apt-get install liblapack-dev libblas-dev libatlas-dev libboost-dev
~~~

Install then Armadillo by replacing *x.y.z* by the downloaded version number:

~~~
  tar xvf armadillo-x.y.z.tar.gz
  cd armadillo-x.y.z/
  ./configure
  make
  sudo make install
~~~

If problems appear concerning boost, you can compile boost from the version available on the website http://www.boost.org/users/history/version_1_53_0.html:

~~~
  tar xvf boost_1_53_0.tar.gz
  cd boost_1_53_0/
  ./bootstrap.sh -prefix=/usr/
  sudo ./b2 --build-type=complete --layout=tagged install
~~~


### 3. InsightToolkit
[Top](#tkdetection)

Download the version available on the website http://www.itk.org/ITK/resources/software.html.

Replace *x.y.z* by the downloaded version number:

~~~
  tar xvf InsightToolkit-x.y.z.tar.gz
  mkdir InsightToolkit-x.y.z/build
  cd InsightToolkit-x.y.z/build
  cmake -DBUILD_EXAMPLES=false -DBUILD_TESTING=false -DITK_BUILD_ALL_MODULES=false -DITKGroup_Core=true -DITKGroup_IO=true ..
  make
  sudo make install
~~~

If a problem appear with *tif_config.h* and/or *tif_dir.h* (typically with ITK 4.3), copy the missing files to the libraries repository:

~~~
  sudo cp  ./Modules/ThirdParty/TIFF/src/itktiff/tif_config.h /usr/local/include/ITK-x.y/itktiff/
  sudo cp ../Modules/ThirdParty/TIFF/src/itktiff/tif_dir.h    /usr/local/include/ITK-x.y/itktiff/
~~~

**Think to change the ITK_NUMBER variable by x.y in the TKDetection.pro file (line 13) !**

### 4. Qwt
[Top](#tkdetection)

Use the version available on the *Main* repository. 

~~~
  sudo apt-get install libqwt-dev
~~~

If problems appear, you can install the version available on the website http://sourceforge.net/projects/qwt/files/qwt/6.0.2/ :

~~~
  tar xvf qwt-6.0.2.tar.bz2
  mkdir qwt-6.0.2/build
  cd qwt-6.0.2/build
  qmake ../qwt.pro
  make
  sudo make install
~~~

### 5. Qxt
[Top](#tkdetection)

Use the version available on the *Main* repository. 

~~~
  sudo apt-get install libqxt-dev
~~~

If the repository version does not install the configuration files *qxt.prf* and *qxtvars.prf* required by TKDetection.pro,
you can install the version available on the website http://www.libqxt.org.


Replace *xxxxxxxxx* by the downloaded file number.

~~~
  tar xvf libqxt-libqxt-xxxxxxxxx.tar.bz2
  cd libqxt-libqxt-xxxxxxxxx/
  ./configure
  make
  sudo make install
~~~

If Qxt widgets does not appear on QtDesigner:

~~~
  sudo ln -s /usr/local/Qxt/lib/libQxtGui.so.0 /usr/lib/
  sudo ln -s /usr/local/Qxt/lib/libQxtCore.so.0 /usr/lib/
~~~
  
Restart QtDesigner.


### 6. QwtPolar
[Top](#tkdetection)

Use the version available on the website http://sourceforge.net/projects/qwtpolar.

Replace *x.y.z* by the downloaded version number:


~~~
  unzip qwtpolar-x.y.z.zip
  mkdir qwtpolar-x.y.z/build
  cd qwtpolar-x.y.z/build
  qmake ..
  make
  sudo make install
~~~

##### If you have a problem during the TKDetection compilation step:

1.  Check that the following files exist:
    - /usr/share/qt4/mkspecs/features/qwtpolar.prf
    - /usr/share/qt4/mkspecs/features/qwtpolarconfig.pri

  If they do not exist:
  ~~~
      sudo ln -s /usr/local/qwtpolar-x.y.z/features/qwtpolar.prf /usr/share/qt4/mkspecs/features/
      sudo ln -s /usr/local/qwtpolar-x.y.z/features/qwtpolarconfig.pri /usr/share/qt4/mkspecs/features/
  ~~~

2.  if 1. does not resolve the problem:

  ~~~
      sudo ln -s /usr/local/qwtpolar-x.y.z/lib/libqwtpolar.so       /usr/lib/
      sudo ln -s /usr/local/qwtpolar-x.y.z/lib/libqwtpolar.so.1     /usr/lib/
      sudo ln -s /usr/local/qwtpolar-x.y.z/lib/libqwtpolar.so.1.0   /usr/lib/
      sudo ln -s /usr/local/qwtpolar-x.y.z/lib/libqwtpolar.so.1.0.0 /usr/lib/
  ~~~

##### If the QwtPolarPlot widget does not appear in QtDesigner

Don't forget to replace *x.y.z* by the downloaded version number:

~~~
  sudo cp /usr/local/qwtpolar-x.y.z/plugins/designer/libqwt_polar_designer_plugin.so /usr/lib/x86_64-linux-gnu/qt4/plugins/designer/
~~~


### 7. DGtalTools
[Top](#tkdetection)

You must install the DGtal library required by DGtalTools.

The DGtal library required QGLViewer, Boost program options and GMP:

~~~
	sudo apt-get install libqglviewer-dev-common libboost-program-options-dev libgmp-dev
~~~


Now you can clone and install DGtal from the main repository:

~~~
  git clone git://github.com/DGtal-team/DGtal.git DGtal
  mkdir DGtal/build
  cd DGtal/build
  cmake .. -DWITH_GMP=true -DWITH_ITK=true -DWITH_QGLVIEWER=true -DBUILD_EXAMPLES=false
  make
  sudo make install
~~~

If a problem appear during the cmake step, add an ITK parameter to the command with the version number of ITK (x.y below):

~~~
  cmake .. -DWITH_GMP=true -DWITH_ITK=true -DWITH_QGLVIEWER=true -DBUILD_EXAMPLES=false -DITK_DIR=/usr/local/lib/cmake/ITK-x.y/
~~~

Install then DGtalTools:

~~~
  git clone git://github.com/DGtal-team/DGtalTools.git DGtalTools
  mkdir DGtalTools/build
  cd DGtalTools/build
  cmake .. -DWITH_VISU3D_QGLVIEWER=true
  make
  sudo make install
~~~

If a problem appear, use the same tips than the DGtal installation:

~~~
  cmake .. -DWITH_VISU3D_QGLVIEWER=true -DWITH_QGLVIEWER=true -DITK_DIR=/usr/local/lib/cmake/ITK-x.y/
~~~

### 8. ImaGene
[Top](#tkdetection)

Kerutils uses features of ImaGene library.

Install ImaGene by using the version without dependencies:

~~~
  git clone git://github.com/kerautret/ImaGene-forIPOL.git
  mkdir ImaGeneNoDep/build
  cd ImaGeneNoDep/build
  cmake ..
  make
  sudo make install
~~~

### 9. KerUtils
[Top](#tkdetection)

KerUtils is the library used to detect the curvature on contour of knots.
It provides the _curvature\_gmcb_ binary file.

KerUtils should be downloaded here: http://www.loria.fr/~krahenbu/TKDetection/KerUtils.zip

~~~
  unzip KerUtils.zip
  mkdir KerUtils/build && cd KerUtils/build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  make
  sudo make install
~~~

TKDetection installation
-------------------------
[Top](#tkdetection)


Begin by check the ITK_NUMBER variable in the TKDetection.pro file (line 13).

Clone the project from the Github repository of this webpage and compile:

~~~
  git clone https://github.com/adrien057/TKDetection.git
  cd TKDetection
  mkdir build && cd build
  qmake ..
  make
~~~

The binary file **TKDetection** is then located in the build/bin directory of TKDetection.
