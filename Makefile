# PYTHON_VERSION = 3.5
# PYTHON_INCLUDE = /usr/include/python$(PYTHON_VERSION)
# location of the Boost Python include files and library
# BOOST_INC = /usr/include
# BOOST_LIB = /usr/lib
BOOST_LOG = -DBOOST_LOG_DYN_LINK -lboost_system -lboost_log -lboost_log_setup
BOOST_FLAG = -lboost_thread 
TINS_FLAG = -ltins
POSIX_FLAG = -lpthread
CPP_OPT = -std=c++11 -g

ALL_OPT = $(CPP_OPT) $(POSIX_FLAG) $(BOOST_FLAG) $(BOOST_LOG) $(TINS_FLAG)
# PYTHON_DEFAULT = /Library/Frameworks/Python.framework/Versions/3.7/include/python3.7m

TARGET = server_main

$(TARGET): $(TARGET).o log_service.o constants.o name_tricks.o response_maker.o tcp_server.o udp_server.o edns.o
	g++ *.o $(ALL_OPT) -o $(TARGET) && mv *.o build

$(TARGET).o : cpp/main.cpp
	g++ -c cpp/main.cpp $(ALL_OPT)

tcp_server.o : cpp/server/tcp_server.cpp cpp/server/tcp_connection.cpp 
	g++ -c cpp/server/tcp_server.cpp cpp/server/tcp_connection.cpp $(ALL_OPT)

udp_server.o : cpp/server/udp_server.cpp
	g++ -c cpp/server/udp_server.cpp $(ALL_OPT)

log_service.o : cpp/log/log_service.cpp
	g++ -c cpp/log/log_service.cpp $(ALL_OPT)

constants.o : cpp/constants.hpp
	g++ -c cpp/constants.hpp $(ALL_OPT)

name_tricks.o : cpp/packet/name_trick.cpp
	g++ -c cpp/packet/name_trick.cpp $(ALL_OPT)

response_maker.o : cpp/packet/response_maker.hpp
	g++ -c cpp/packet/response_maker.hpp $(ALL_OPT)

edns.o : cpp/packet/edns.cpp
	g++ -c cpp/packet/edns.cpp $(CPP_OPT) $(TINS_FLAG) 

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
