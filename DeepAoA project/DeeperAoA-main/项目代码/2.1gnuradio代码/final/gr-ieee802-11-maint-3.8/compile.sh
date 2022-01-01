sudo mkdir build
cd build
sudo make uninstall
sudo ldconfig
cd ..
rm -rf build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
sudo ldconfig

