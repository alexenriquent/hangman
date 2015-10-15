## Hangman ##

In the game of Hangman the server randomly selects a two word phrase from the data source. The word phrase is represented by a row of dashes representing each letter of the word on the client machine. If the client selects a correct letter the letter should then appear in all the correct places in which it occurs.

The client has a number of turns in which to guess all the letters in the two word phrase. The number of turns the client receives is proportional to the length of the two word phrase. If the client guesses all the letters before running out of turns the client wins the game. If the client fails to guess all the letters within the number of turns provided the client loses the game.

Compilation Instructions

# Server #

Files: server.h and server.c
Compilation command: c99 -o server server.c -lpthread
Run command: ./server <port number> (e.g. ./server 8080) 
            if no port number provided (i.e. ./server), the server will listening on port 12345.


# Client #

Files: client.h and client.c
Compilation command: c99 -o client client.c 
Run command: ./client 127.0.0.1 <port number> (e.g. ./client 127.0.0.1 8080) 
            Client needs to explicitly define a port number.