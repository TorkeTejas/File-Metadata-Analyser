CXX := g++

CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic -I/path/to/rapidxml/include

LIBS := -lpoppler-cpp -lzip

SRCDIR := src
INCDIR := include
BUILDDIR := build
BINDIR := bin

TARGET := $(BINDIR)/file_metadata_analyzer

SRCEXT := cpp

SOURCES := $(wildcard $(SRCDIR)/*.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $^ -o $(TARGET) $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -MMD -MP -c -o $@ $<

clean:
	$(RM) -r $(BUILDDIR) $(BINDIR)

-include $(DEPS)