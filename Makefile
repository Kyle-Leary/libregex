SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

TARGET := libregex.a
TEST := regextest 

INCLUDES := -Iapi
CFLAGS := $(INCLUDES) -ggdb

all: $(TARGET)
	$(info OBJ: $(OBJ))

$(TARGET): $(OBJ)
	@echo $@ $^
	ar rcs $@ $^

build/%.o: src/%.c
	gcc -c -o $@ $< $(CFLAGS)

$(TEST): $(TARGET)
	gcc -o $(TEST) test.c $(TARGET) $(CFLAGS)

run: $(TEST)
	./$(TEST)

debug: $(TEST)
	gf2 -ex "run ./$(TEST)"

clean: 
	rm $(OBJ) $(TARGET) $(TEST)

.PHONY: $(TARGET) all test clean
