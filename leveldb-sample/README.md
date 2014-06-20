"leveldb-test": a work in progress used for debugging Shadow plug-ins
=========================

Manual Dependencies 
===================
1. leveldb source https://leveldb.googlecode.com/files/leveldb-1.15.0.tar.gz
   Untar and build.
   Write down LEVELDB_ROOT=path/to/leveldb-1.15.0
   
2. git clone -b shadow https://github.com/amiller/gnu-pth
   - This should be copied or symlinked to shadow-plugin-extras/gnu-pth
   - Run ./configure --enable-epoll --enable-debug

3. mkdir build; cd build
   CC="clang" CXX="clang++" LEVELDB_ROOT=path/to/leveldb-1.15.0 cmake ..

4. make generate_pth && make install

5. mkdir -p init_data

6. ../leveldb-sample/shadow-leveldb-yank ../leveldb-sample/example-yank.xml -y




copyright holders
-----------------

No copyright is claimed by the United States Government.

licensing deviations
--------------------

No deviations from LICENSE.

last known working version
--------------------------

This plug-in was last tested and known to work with 
Shadow v1.9.0
commit 2fb316ad84801434c4b5e0536740807774c732fd
Date:   Tue Mar 11 18:20:36 2014 -0400
