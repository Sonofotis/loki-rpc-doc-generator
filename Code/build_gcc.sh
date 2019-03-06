mkdir -p ../Bin
pushd ../Bin

g++ -std=c++14 -O2 -o loki_rpc_doc_generator ../Code/loki_rpc_doc_generator.cpp
../Bin/loki_rpc_doc_generator ~/loki/src/rpc/core_rpc_server_commands_defs.h > output2.md

popd
