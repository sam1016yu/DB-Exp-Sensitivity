#!/bin/bash


/bin/cp /etc/skel/.bashrc ~/
source ~/.bashrc
sudo apt update
sudo apt install -y libprotobuf-dev libevent-dev libssl-dev \
        protobuf-compiler cmake libgtest-dev openjdk-11-jdk maven
sudo cd /usr/src/gtest/
sudo cmake CMakeLists.txt
sudo make
# copy or symlink libgtest.a and libgtest_main.a to your /usr/lib folder
sudo cp *.a /usr/lib
cd $HOME/tapir   #suppose tapir is in home directory,need to change accordingly
make
echo "export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64" >> ~/.bashrc
source ~/.bashrc
