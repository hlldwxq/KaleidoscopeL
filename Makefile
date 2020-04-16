C_SOURCES = $(shell find . -name "*.cpp")
C_OBJECTS = $(patsubst %.cpp, %.o, $(C_SOURCES))
C_FLAGS = `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native orcjit` -rdynamic

start: $(C_OBJECTS)
	clang++ -g $(C_OBJECTS) $(C_FLAGS) -o Kaleidoscope
.cpp.o:
	@echo Compiling cpp source code files $< ...
	clang++ -g -c $< $(C_FLAGS) -o $@


clean:
	$(RM) $(C_OBJECTS)