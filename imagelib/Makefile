CXXFLAGS = -std=c++17 -lstdc++fs -Wall -Wextra -Wcast-align -Wcast-qual \
					 -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self \
					 -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
					 -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls \
					 -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel \
					 -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused
LDLIBS = -ltiff -ljpeg

HEADER = ia_tiff.h
SOURCES = ia_tiff.cpp
OBJS = $(SOURCES:.cpp=.o)

TARGET = libiatiff.so

.PHONY: all clean tclean

all: $(TARGET)

$(TARGET): $(HEADER) $(OBJS)
	g++ $(CXXFLAGS) $(OBJS) -shared -o $(TARGET) $(LDLIBS)

%.o: %.cpp $(HEADER)
	g++ $(CXXFLAGS) -fPIC -O3 -c $< -o $@ -I./ $(LDLIBS)

clean:
	rm *.o; rm $(TARGET)

db:
	compiledb -n make all

test:
	g++ $(CXXFLAGS) test.cpp $(HEADER) $(SOURCES) -o test $(LDLIBS)

tclean:
	rm -r test data_out
