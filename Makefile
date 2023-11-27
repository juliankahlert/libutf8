CC := $(shell command -v cgcc)
CHECK := $(shell command -v smatch) --pedantic --full-path
BUILD_DIR ?= release

CFLAGS += -Wall                    \
          -Werror                  \
          -flto                    \
          -fdata-sections          \
          -ffunction-sections

LDFLAGS += -Wl,--gc-sections

SP_FLAGS += -Werror                \
            -Wsparse-all           \
            -Wno-unknown-attribute

export CC
export CHECK
export BUILD_DIR

export CFLAGS
export LDFLAGS
export SP_FLAGS

.PHONY: rm all clean release debug

all:
	make BUILD_DIR=release release
	make BUILD_DIR=debug debug

clean:
	make BUILD_DIR=release rm
	make BUILD_DIR=debug rm

release: CFLAGS += -O3
release: $(BUILD_DIR)/test

debug: CFLAGS += -O0 -g -fsanitize=undefined,address
debug: $(BUILD_DIR)/test

$(BUILD_DIR)/test: $(BUILD_DIR)/test.o $(BUILD_DIR)/libutf8.a
	mkdir --parents $$(dirname $@)
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^

$(BUILD_DIR)/libutf8.a: $(BUILD_DIR)/utf8.o
	mkdir --parents $$(dirname $@)
	ar rcs $@ $<

$(BUILD_DIR)/utf8.o: src/utf8.c src/utf8.h
	mkdir --parents $$(dirname $@)
	sparse $(SP_FLAGS) $<
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/test.o: src/test.c src/utf8.h
	mkdir --parents $$(dirname $@)
	sparse $(SP_FLAGS) $<
	$(CC) $(CFLAGS) -c -o $@ $<

rm:
	rm --force $(BUILD_DIR)/*.o
	rm --force $(BUILD_DIR)/*.a
	rm --force $(BUILD_DIR)/test

	test "$(BUILD_DIR)" != "src" \
		&& rm --recursive --force $(BUILD_DIR)

