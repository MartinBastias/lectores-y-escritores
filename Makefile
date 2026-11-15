CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread
TARGET   = rw
SRCS     = rw.cpp monitor_rw.cpp control_atomico_rw.cpp verificador.cpp
HDRS     = control_rw.hpp monitor_rw.hpp control_atomico_rw.hpp verificador.hpp

.PHONY: all clean tsan

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

tsan: $(SRCS) $(HDRS)
	$(CXX) -std=c++17 -O1 -g -Wall -Wextra -pthread -fsanitize=thread $(SRCS) -o $(TARGET)_tsan

clean:
	rm -f $(TARGET) $(TARGET).exe $(TARGET)_tsan $(TARGET)_tsan.exe *.o
