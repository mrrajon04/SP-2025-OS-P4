CC = cc
CFLAGS = -Wall -Wextra -MMD -MP
LDFLAGS = -lpthread

SRC_DIR = src
APP_DIR = app
BUILD_DIR = build

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
APP_FILES = $(wildcard $(APP_DIR)/*.c)
OBJ_FILES = $(patsubst %.c, $(BUILD_DIR)/%.c.o, $(SRC_FILES) $(APP_FILES))
DEP_FILES = $(OBJ_FILES:.o=.d)

TARGET = myprogram

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

check: $(TARGET)
	./$(TARGET)

-include $(DEP_FILES)
	sudo apt-get install -y libio-socket-ssl-perl libmime-tools-perl


-include $(DEPS) $(TEST_DEPS) $(EXE_DEPS)
