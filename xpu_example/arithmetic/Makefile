CC = gcc
CFLAGS = -fPIC -shared 
SOURCES = $(wildcard src/*.c)
OBJ = $(patsubst %.c, %.o, $(SOURCES))
TARGET = arithmetic

ifeq (y,$(HAVE_XPU))
    CFLAGS += -DHAVE_XPU
endif

#link
$(TARGET):$(OBJ)
	@$(CC) $(OBJ) $(LIB_PATH) $(LIB_NAMES) $(CFLAGS) -o lib/lib$(TARGET).so
	@rm -rf $(OBJ)
 
#compile
%.o: %.c
	@$(CC) $(INCLUDES) -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	@rm -rf $(OBJ) $(TARGET) lib/lib$(TARGET).so

