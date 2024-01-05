LIB_TARGET_STATIC := libcyberiadaml.a
LIB_TARGET_DYNAMIC := libcyberiadaml.so

ifeq ($(DYNAMIC), 1)
    LIB_TARGET := $(LIB_TARGET_DYNAMIC)
else
    LIB_TARGET := $(LIB_TARGET_STATIC)
endif

TEST_TARGET := cyberiada_test
LIB_SOURCES := cyberiadaml.c
TEST_SOURCES := test.c
LIB_OBJECTS := $(patsubst %.c, %.o, $(LIB_SOURCES))
TEST_OBJECTS := $(patsubst %.c, %.o, $(TEST_SOURCES))

ifeq ($(DEBUG), 1)
    CFLAGS := -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion -fPIC -g3 -D__DEBUG__
else
    CFLAGS := -fPIC
endif

INCLUDE := -I. -I/usr/include/libxml2
LIBS := -L/usr/lib -lxml2
TEST_LIBS := -L. -lcyberiadaml

$(LIB_TARGET): $(LIB_OBJECTS)
ifeq ($(DYNAMIC), 1)
	gcc -shared $(LIBS) $(LIB_OBJECTS) -o $@
else
	ar rcs $@ $(LIB_OBJECTS)
endif

$(TEST_TARGET): $(TEST_OBJECTS) $(TARGET)
	gcc $(TEST_OBJECTS) -Wl,--no-as-needed $(LIBS) $(TEST_LIBS) -o $@

%.o: %.c
	gcc -c $< $(CFLAGS) $(INCLUDE) -o $@

clean:
	rm -f *~ *.o $(TARGET) $(TEST_TARGET) $(LIB_TARGET_STATIC) $(LIB_TARGET_DYNAMIC)

test: $(TEST_TARGET)

all: $(LIB_TARGET) $(TEST_TARGET)

.PHONY: all clean test
