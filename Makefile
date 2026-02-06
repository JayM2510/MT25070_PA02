# Roll No: MT25070
# Graduate Systems PA02

CC = gcc
CFLAGS = -O2 -pthread

all:
	$(CC) $(CFLAGS) MT25070_Part_A1_Server.c -o MT25070_A1_Server
	$(CC) $(CFLAGS) MT25070_Part_A1_Client.c -o MT25070_A1_Client
	$(CC) $(CFLAGS) MT25070_Part_A2_Server.c -o MT25070_A2_Server
	$(CC) $(CFLAGS) MT25070_Part_A1_Client.c -o MT25070_A2_Client
	$(CC) $(CFLAGS) MT25070_Part_A3_Server.c -o MT25070_A3_Server
	$(CC) $(CFLAGS) MT25070_Part_A1_Client.c -o MT25070_A3_Client

clean:
	rm -f MT25070_A*_Server MT25070_A*_Client
