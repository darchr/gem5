export SST_CORE_HOME=`pwd`
wget https://github.com/sstsimulator/sst-core/releases/download/v13.0.0_Final/sstcore-13.0.0.tar.gz
tar xf sstcore-13.0.0.tar.gz
cd sstcore-13.0.0
./configure --prefix=$SST_CORE_HOME --with-python=/usr/bin/python3-config
make all -j32
make install
cd ..
wget https://github.com/sstsimulator/sst-elements/releases/download/v13.0.0_Final/sstelements-13.0.0.tar.gz
tar xf sstelements-13.0.0.tar.gz
cd sst-elements-library-13.0.0
./configure --prefix=$SST_CORE_HOME --with-python=/usr/bin/python3-config  --with-sst-core=$SST_CORE_HOME
make all -j32
make install
cd ..
export PKG_CONFIG_PATH=`pwd`/lib/pkgconfig
make clean ; make ARCH=ARM -j4
