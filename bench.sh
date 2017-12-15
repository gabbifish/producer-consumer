##########
# pthreads
##########
# build
cd ./pc-c/pc-pthreads/
make clean
make

# producer bound
time ./pc-pthreads -e 100 -i
time ./pc-pthreads -e 1000 -i
time ./pc-pthreads -e 10000 -i

# consumer bound
time ./pc-pthreads -e 100 -j
time ./pc-pthreads -e 1000 -j
time ./pc-pthreads -e 10000 -j

# go back to root dir
cd ../../

##########
# libmill
##########
# build
cd ./pc-c/pc-libmill/
make clean
make

# producer bound
time ./pc-libmill -e 100 -i
time ./pc-libmill -e 1000 -i
time ./pc-libmill -e 10000 -i

# consumer bound
time ./pc-libmill -e 100 -j
time ./pc-libmill -e 1000 -j
time ./pc-libmill -e 10000 -j

# go back to root dir
cd ../../

######
# rust
######
# build
cd ./pc-rust/
cargo clean
cargo build --release

# producer bound
time ./target/release/pc-rust -e 100 -i
time ./target/release/pc-rust -e 1000 -i
time ./target/release/pc-rust -e 10000 -i

# consumer bound
time ./target/release/pc-rust -e 100 -j
time ./target/release/pc-rust -e 1000 -j
time ./target/release/pc-rust -e 10000 -j

# go back to root dir
cd ../

