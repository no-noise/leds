CXX :=			g++

FLAGS :=		-std=c++14 -pthread -Os -gdwarf-4 -march=nocona \
				-fno-strict-aliasing -fno-inline -fno-omit-frame-pointer \
				-Wall -Wextra -Wpedantic -Wshadow -Wcast-align -Wcast-qual \
				-Wconversion -Wsign-conversion -Wstrict-overflow=4 \
				-Wtrampolines -Wmissing-declarations -Wredundant-decls \
				-Wformat=2 -D_FORTIFY_SOURCE=2 -fstack-protector-all

CXXFLAGS :=		$(FLAGS) -Wa,--noexecstack
LDFLAGS :=		$(FLAGS) -Wl,-z,relro,-z,now,-z,noexecstack

DIR :=			$(shell pwd)
OBJS :=			test.o
EXE :=			test

%.o:			%.cpp
				$(CXX) $(CXXFLAGS) -c -o $@ $<

$(EXE):			$(OBJS)
				$(CXX) $(LDFLAGS) -o $(EXE) $(OBJS)

val:			$(EXE)
				valgrind \
					--leak-check=full --leak-resolution=high \
					--show-leak-kinds=all --keep-stacktraces=alloc-and-free \
					$(DIR)/$(EXE) ping

run:			$(EXE)
				$(DIR)/$(EXE) ping

clean:
				rm -f $(EXE) $(OBJS)
