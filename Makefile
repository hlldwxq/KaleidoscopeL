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