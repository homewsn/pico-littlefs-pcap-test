#

TARGET := pico-littlefs-pcap-test
OBJDIR := obj
SRCDIR := src
HDRDIR := src
SOURCES := $(wildcard $(SRCDIR)/*.c)
HEADERS := $(wildcard $(SRCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS := $(HEADERS)

INCLPATH := -I$(HDRDIR)
LIBS := -lpcap -lm
CFLAGS := -g -DENABLE_TRACE

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS += -lrt
endif


# main goal
all: $(TARGET)

# target executable
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBPATH) $(LIBS)

# object files
$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(DEPS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLPATH)

# create object files directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# clean
clean:
	rm -rf $(OBJDIR)

# distclean
distclean: clean
	rm -f $(TARGET)

.PHONY: all clean distclean
