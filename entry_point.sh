udp_scanner() {
    ./udp_scanner --file_path=/home/jxm959/ip_shuffle.txt
    # ./udp_scanner --file_path=cpp/scanner/address_cpy.txt
}

tcp_scanner() {
    ./tcp_scanner
}

exit_handler() {
    kill -INT $(ps -o pgid= $PID | grep -o '[0-9]*')
}

trap 'exit_handler' INT

udp_scanner & tcp_scanner