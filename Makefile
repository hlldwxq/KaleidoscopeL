C_SOURCES = $(shell find . -name "*.cpp")
C_OBJECTS = $(patsubst %.cpp, %.o, $(C_SOURCES))
C_FLAGS = `llvm-config-10 --cxxflags --ldflags --system-libs --libs core mcjit native orcjit` -rdynamic

start: $(C_OBJECTS)
	clang++-10 -g $(C_OBJECTS) $(C_FLAGS) -o Kaleidoscope
.cpp.o:
	@echo Compiling cpp source code files $< ...
	clang++-10 -g -c $< $(C_FLAGS) -o $@


clean:
	$(RM) $(C_OBJECTS)

file.ll: Kaleidoscope file.k
	./Kaleidoscope > file.ll 2>&1 || echo "Ignoring Kaleidoscope failure"
	sed -i '/^Segmentation fault/d' file.ll

interface_file: interface_file.c file.ll
	llvm-as-10 -disable-output file.ll  # Just a well-formedness check, as clang tends to simply segfault on malformed llvm code
	clang-10 -Wall -Wextra -g -o interface_file interface_file.c file.ll
