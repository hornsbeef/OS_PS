CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -pthread -Wno-return-local-addr -g
LDFLAGS += -pthread

TARGET = task1

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(TARGET) *.o

.PHONY: valg
valg:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./task1 some Arguments 4297
