# PYTHON_VERSION = 3.5
# PYTHON_INCLUDE = /usr/include/python$(PYTHON_VERSION)
# location of the Boost Python include files and library
# BOOST_INC = /usr/include
# BOOST_LIB = /usr/lib
BOOST_LOG = -DBOOST_LOG_DYN_LINK -lboost_system -lboost_log -lboost_log_setup -lboost_program_options
BOOST_FLAG = -lboost_thread
TINS_FLAG =  -ltins
POSIX_FLAG = -lpthread -lrt
CPP_OPT = -std=c++11 -g
OPTIMIZATION = -O3

ALL_OPT = $(CPP_OPT) $(OPTIMIZATION) $(BOOST_FLAG) $(BOOST_LOG) $(TINS_FLAG) $(POSIX_FLAG)
# PYTHON_DEFAULT = /Library/Frameworks/Python.framework/Versions/3.7/include/python3.7m

TARGET_SERVER = server_main
TARGET_SCANNER = scanner_main

$(TARGET_SERVER): $(TARGET_SERVER).o log_service.o constants.o name_utils.o response_maker.o tcp_server.o udp_server.o edns.o
	g++ *.o $(ALL_OPT) -o $(TARGET_SERVER) && mv *.o build

$(TARGET_SERVER).o : cpp/server/server_main.cpp
	g++ -c cpp/server/server_main.cpp $(ALL_OPT)

tcp_server.o : cpp/server/tcp_server.cpp cpp/server/tcp_connection.cpp 
	g++ -c cpp/server/tcp_server.cpp cpp/server/tcp_connection.cpp $(ALL_OPT)

udp_server.o : cpp/server/udp_server.cpp
	g++ -c cpp/server/udp_server.cpp $(ALL_OPT)

log_service.o : cpp/log/log_service.cpp
	g++ -c cpp/log/log_service.cpp $(ALL_OPT)

constants.o : cpp/constants.hpp
	g++ -c cpp/constants.hpp $(ALL_OPT)

name_utils.o : cpp/packet/name_util.cpp
	g++ -c cpp/packet/name_util.cpp $(ALL_OPT)

response_maker.o : cpp/packet/response_maker.hpp
	g++ -c cpp/packet/response_maker.hpp $(ALL_OPT)

edns.o : cpp/packet/edns.cpp
	g++ -c cpp/packet/edns.cpp $(CPP_OPT) $(TINS_FLAG) 

server: $(TARGET_SERVER) ;

udp_scanner_sender.o : cpp/scanner/udp_scanner_sender.cpp
	g++ -c cpp/scanner/udp_scanner_sender.cpp $(ALL_OPT)

udp_scanner_listener.o : cpp/scanner/udp_scanner_listener.cpp
	g++ -c cpp/scanner/udp_scanner_listener.cpp $(ALL_OPT)

udp_scanner.o : cpp/scanner/udp_scanner.cpp
	g++ -c cpp/scanner/udp_scanner.cpp $(ALL_OPT)

tcp_scanner.o : cpp/scanner/tcp_scanner.cpp
	g++ -c cpp/scanner/tcp_scanner.cpp $(ALL_OPT)

monitor.o : cpp/scanner/monitor.cpp
	g++ -c cpp/scanner/monitor.cpp $(ALL_OPT)

packet_factory.o: cpp/packet/packet_factory.cpp
	g++ -c cpp/packet/packet_factory.cpp $(CPP_OPT) $(TINS_FLAG)

udp_scanner_main: udp_scanner_sender.o udp_scanner_listener.o udp_scanner.o name_utils.o log_service.o token_bucket.o tcp_scanner.o monitor.o packet_factory.o
	g++ udp_scanner_sender.o \
	    udp_scanner_listener.o \
		udp_scanner.o \
		log_service.o \
		name_util.o \
		token_bucket.o \
		tcp_scanner.o \
		monitor.o \
		packet_factory.o \
		$(ALL_OPT) -o \
		udp_scanner

tcp_scanner_main: tcp_scanner.o log_service.o name_utils.o 
	g++ tcp_scanner.o log_service.o name_util.o -o tcp_scanner $(ALL_OPT)

token_bucket.o: cpp/scanner/token_bucket.cpp
	g++ -c cpp/scanner/token_bucket.cpp $(CPP_OPT) $(POSIX_FLAG)

scanner: udp_scanner_main
	mv *.o build

# TARGET = sender

# $(TARGET).so: $(TARGET).o dns_packet.o udp_header.o measurement_misc.o
# 	g++ -shared -Wl,-install_name,$(TARGET).so -o $(TARGET).so\
# 	 $(TARGET).o \
# 	 dns_packet.o \
# 	 udp_header.o \
# 	 measurement_misc.o \
# 	-L$(BOOST_LIB) -lboost_python -L/Library/Frameworks/Python.framework/Versions/3.7/lib/python3.7/config-3.7m-darwin -lpython$(PYTHON_VERSION) 

# $(TARGET).o: $(TARGET).cpp
# 	g++ -std=c++11 -I$(PYTHON_DEFAULT) -I$(BOOST_INC) -fPIC -c $(TARGET).cpp -o $(TARGET).o

# dns_packet.o: dns_packet.cpp
# 	g++ -std=c++11 -I$(PYTHON_DEFAULT) -I$(BOOST_INC) -fPIC -c dns_packet.cpp -o dns_packet.o

# udp_header.o: udp_header.cpp
# 	g++ -std=c++11 -I$(PYTHON_DEFAULT) -I$(BOOST_INC) -fPIC -c udp_header.cpp -o udp_header.o

# measurement_misc.o: measurement_misc.cpp
# 	g++ -std=c++11 -I$(PYTHON_DEFAULT) -I$(BOOST_INC) -fPIC -c measurement_misc.cpp -o measurement_misc.o

# install:
# 	mv -f $(TARGET).so  /usr/lib/python3/dist-packages

clean:
	rm -f *.o build/*.o $(TARGET)
