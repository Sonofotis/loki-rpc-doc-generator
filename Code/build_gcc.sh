mkdir -p ../Bin
pushd ../Bin

g++ -std=c++14 -O2 -o loki_rpc_doc_generator ../Code/loki_rpc_doc_generator.cpp

popd
