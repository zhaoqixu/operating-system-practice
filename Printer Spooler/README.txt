To compile clients.c and printer.c you should use command " gcc -o clients clients.c -lpthread -lrt" & "gcc -o printer printer.c -lpthread -lrt".

To use clients, you should use command "./clients num1 num2" num1 is clientID; num2 is how many pages to be printed which is also the job duration.

To use printer, you should use command "./printer num3" num3 is the slots number in the joblist.
