CXXFLAGS =	-O2 -g -Wall -fmessage-length=0
LIBS = -lpthread
GCC = arm-linux-g++
#GCC = g++


SRC += *.cpp 
SRC += command_function/*.cpp 
SRC += command_function/uart_module/*.cpp
SRC += command_function/net_module/*.cpp
SRC += command_function/key_module/*.cpp
SRC += oled/*.cpp

LIBS =

TARGET =	capture.o

$(TARGET):	$(SRC)
	$(GCC)  $(SRC) -o  $(TARGET) -lpthread

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
