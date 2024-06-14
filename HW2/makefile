# Define the compiler
CXX = g++

# Define the compiler flags
CXXFLAGS = -Wall -g

# Define the target executables
TARGET = fileSystemOper
CREATE_TARGET = makeFileSystem

# Define the source and object files
SRCS = src/main.cpp src/filesystem.cpp src/operations.cpp src/utils.cpp
OBJDIR = object
OBJS = $(SRCS:src/%.cpp=$(OBJDIR)/%.o)

# Default rule to build both targets
all: $(TARGET) $(CREATE_TARGET)

# Rule to link the object files into the main target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile the source files into object files
$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure the object directory exists
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Rule to compile and link fs_create.cpp into fs_create executable
$(CREATE_TARGET): src/fs_create.cpp src/filesystem.cpp| $(OBJDIR)
	$(CXX) $(CXXFLAGS) src/fs_create.cpp src/filesystem.cpp -o $(CREATE_TARGET)

# Rule to clean up the build directory
clean:
	rm -f $(OBJS) $(TARGET) $(CREATE_TARGET)

# Phony targets to avoid conflicts with files named 'clean' or 'all'
.PHONY: clean all
