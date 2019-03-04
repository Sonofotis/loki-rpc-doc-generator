# doc-parser
Designed to auto-generate documentation for 'core_rpc_server_commands_defs.h' in loki-project/loki. 
This parser will generate two files:
- documentation.txt: A file that keeps track of tokens that are processed by the parser
- docs.html: The HTML file featuring a rudimentary description of input and output variables for each RPC method

See 'Issues' for a description of all notable drawbacks of this parser as is.

The ultimate goal of this parser is to generate documentation identical to the standard of that on:
https://loki-project.github.io/loki-docs/Developer/DaemonRPCGuide/
