##########
# pthreads
##########
# build
cd ./pc-c/pc-pthreads/
make clean
make

# producer bound
echo "pc-pthreads: producer-bound"
time ./pc-pthreads -e 100 -i
time ./pc-pthreads -e 1000 -i
time ./pc-pthreads -e 10000 -i

# consumer bound
echo "pc-pthreads: consumer-bound"
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
echo "pc-libmill: producer-bound"
time ./pc-libmill -e 100 -i
time ./pc-libmill -e 1000 -i
time ./pc-libmill -e 10000 -i

# consumer bound
echo "pc-libmill: consumer-bound"
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
echo "pc-rust: producer-bound"
time ./target/release/pc-rust -e 100 -i
time ./target/release/pc-rust -e 1000 -i
time ./target/release/pc-rust -e 10000 -i

# consumer bound
echo "pc-rust: consumer-bound"
time ./target/release/pc-rust -e 100 -j
time ./target/release/pc-rust -e 1000 -j
time ./target/release/pc-rust -e 10000 -j

# go back to root dir
cd ../

####
# go
####
# build
cd ./pc-go/
rm producer-consumer
go build producer-consumer.go

# producer bound
echo "pc-go: producer-bound"
time ./producer-consumer -e 100 -i
time ./producer-consumer -e 1000 -i
time ./producer-consumer -e 10000 -i

# consumer bound
echo "pc-go: consumer-bound"
time ./producer-consumer -e 100 -j
time ./producer-consumer -e 1000 -j
time ./producer-consumer -e 10000 -j

# go back to root dir
cd ../
