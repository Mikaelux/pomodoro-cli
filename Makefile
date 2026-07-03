CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncursesw -lmenuw -lpthread
TARGET = pomodoro
SRC = main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -f /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: install uninstall clean
