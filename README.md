local-dns
=========

Your goal is to implement a DNS resolver that runs over UDP. The user inputs strings (either hostnames or IP addresses) that need to be resolved through DNS. Your program must directly use UDP and parse DNS responses without using any shortcuts from Platform SDK. The answers returned by the local DNS server must be displayed to the user including any additional records and multiple answers. A complete specification of packet headers and the various fields is contained in RFCs 1034-1035.
The program should be able to run in two modes – interactive (i.e., using command-line input) and batch (i.e., using input file dns-in.txt). In the former case, the code will return a detailed answer to the query provided by the user (see examples below). In the latter case, the code will read the input file (one question per line) and perform lookups using N threads, where N is specified in the command line. To distinguish between the modes, check if the first argument to the program is an integer. If so, assume this integer is the number of threads for batch lookups. Otherwise, assume the interactive mode.
local dns
