#! /bin/sh
git pull
sudo make clean
sudo make -j4 EPD=epd7in3f
sudo ./epd