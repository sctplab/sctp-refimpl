all:	sender receiver

sender:	sender.o
	gcc -Wall sender.o -L /usr/local/v6/lib -l sctp -o sender
#	gcc -Wall sender.o -o sender

sender.o:	sender.c
	gcc -Wall -g -c sender.c -o sender.o
receiver:	receiver.o
	gcc -Wall receiver.o -L /usr/local/v6/lib  -l sctp -o receiver
#	gcc -Wall receiver.o -o receiver
receiver.o:	receiver.c
	gcc -Wall -g -c receiver.c -o receiver.o
clean:
	rm *.o
setup:
	cp sender sendertest/
	cp receiver receivertest/
