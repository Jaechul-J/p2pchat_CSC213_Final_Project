#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include<fcntl.h> 
#include<errno.h> 
#include "socket.h"
#include "ui.h"

#define MAX_MESSAGE_LENGTH 2048

// Encryption keys
char keys[6];

// Flag for each user
bool isReceiver = false;

// Keep the username in a global so we can access it from the callback
const char* username;

// Create an array of neighbors
int* neighbors = NULL;

// Number of neighbors
int numNeighbors = 0; 

// Function declaration
int send_message(int fd, char* message);

// Function declaration
void* client_fn(void* arg);

// Generate a random key of length 5 that are capital letters
void gen_random(char* s) {
  time_t t;
  srand((unsigned) time(&t));
  char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int i = 0; i < 5; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

  s[5] = '\0';
}
 
// Returns the encrypted text generated with the help of the key
char* cipherText(char* cipher_text, const char* str, char* key)
{
 
    for (int i = 0; i < strlen(str); i++)
    {
      // Keep the whitespace as it is in the ciphertext.
      if(str[i] == ' '){
        cipher_text[i] = ' ';
        continue;
      }
        // Converting in range 0-25
        int x = (str[i] + key[i%5]) %26;
        // Convert into alphabets(ASCII)
        x += 'A';
        cipher_text[i] = (char)x;
    }
    return cipher_text;
}
 
// Decrypts the encrypted text and returns the original textmessage
char* originalText(char* original_text, char* cipher_text, char* key)
{
    int i;
    for (i = 0 ; i < strlen(cipher_text); i++)
    {
      // Keep the whitespace as it is.
      if(cipher_text[i] == ' '){
          original_text[i] = ' ';
          continue;
      }
      // Converting in range 0-25
      int x = (cipher_text[i] - key[i%5] + 26) % 26;
 
      // Convert into alphabets(ASCII)
      x += 'A';
      original_text[i] = (char)(x);
    }
    original_text[i] = '\0';
    return original_text;
}

// This function is run whenever the user hits enter after typing a message
void input_callback(const char* message) {
  // Create a file descriptor that points to secret.txt file
  int fd = open("secret.txt", O_RDWR | O_CREAT | O_TRUNC);

  // Write the user's key in secret.txt
  write(fd, keys, strlen(keys));

  if (strcmp(message, ":quit") == 0 || strcmp(message, ":q") == 0) {
    ui_exit();
  }
  // Start receiving decrypted message
  else if(strcmp(message, ":true") == 0){
    isReceiver = true;
  }
  // Stop receiving decrypted message
  else if(strcmp(message, ":false") == 0){
    isReceiver = false;
  }
  else {
    char cipher_text[strlen(message)];
    // Concatnate username and message
    char newMessage[MAX_MESSAGE_LENGTH];
    strcpy(newMessage, username);
    strcat(newMessage, ":");
    cipherText(cipher_text, message, keys);
    cipher_text[strlen(message)] = '\0';
    strcat(newMessage, cipher_text);
    ui_display(username, message);

    // Send the concatenated message to all neighbors
    for (int i = 0; i < numNeighbors; i++){
      send_message(neighbors[i], newMessage);
    }
  }
  close(fd);
}

pthread_mutex_t mutex;

// Adds to a dynamic array when a new user connects
void add_neighbor(int socket){
  pthread_mutex_lock(&mutex);
  numNeighbors++;
  neighbors = (int*)realloc(neighbors, sizeof(int)*numNeighbors);
  neighbors[numNeighbors-1] = socket;
  pthread_mutex_unlock(&mutex);
}

// Deletes from a dynamic array when a user disconnects
void delete_neighbor(int socket){
  int index = 0;
  while(neighbors[index] != socket) {
    index++;
  }
  int i;
  for (i = index; i < numNeighbors - 1; i++){
    neighbors[i] = neighbors[i+1];
  }
  numNeighbors--;
  neighbors = (int*)realloc(neighbors, sizeof(int)*numNeighbors);
}

// Function that accepts connection from peers
void* server_fn(void* arg){
  int socket_fd = *(int*)arg;
  // neighbors = (int*)malloc(sizeof(int));

  // Wait for a peer to connect
  while(1){
    int peer_socket_fd = server_socket_accept(socket_fd);
    if (peer_socket_fd == -1) {
      perror("accept failed");
      exit(EXIT_FAILURE);
    }

    add_neighbor(peer_socket_fd);

    // Create a new thread to listen to the its neighbor
    pthread_t client_thread;
    if(pthread_create(&client_thread, NULL, client_fn, &peer_socket_fd) != 0){
      perror("pthread_create failed");
      exit(EXIT_FAILURE);
    }
  }

  return NULL;
}

// Send a across a socket with a header that includes the message length.
int send_message(int fd, char* message) {
  // If the message is NULL, set errno to EINVAL and return an error
  if (message == NULL) {
    errno = EINVAL;
    return -1;
  }
  // First, send the length of the message in a size_t
  size_t len = strlen(message);
  if (write(fd, &len, sizeof(size_t)) != sizeof(size_t)) {
    // Writing failed, so return an error
    return -1;
  }

  // Now we can send the message. Loop until the entire message has been written.
  size_t bytes_written = 0;
  char cipher_text[strlen(message)];
  while (bytes_written < len) {
    // Try to write the entire remaining message
    ssize_t rc = write(fd, message + bytes_written, len - bytes_written);

    // Did the write fail? If so, return an error
    if (rc <= 0) return -1;

    // If there was no error, write returned the number of bytes written
    bytes_written += rc;
  }

  return 0;
}

// Receive a message from a socket and return the message string (which must be freed later)
char* receive_message(int fd) {
  // First try to read in the message length
  size_t len;
  if (read(fd, &len, sizeof(size_t)) != sizeof(size_t)) {
    return NULL;
  }

  // Now make sure the message length is reasonable
  if (len > MAX_MESSAGE_LENGTH) {
    errno = EINVAL;
    return NULL;
  }

  // Allocate space for the message
  char* result = malloc(len + 1);

  // Try to read the message. Loop until the entire message has been read.
  size_t bytes_read = 0;

  while (bytes_read < len) {
    // Try to read the entire remaining message
    ssize_t rc = read(fd, result + bytes_read, len - bytes_read);

    // Return an error if read fails
    if (rc <= 0) {
      free(result);
      return NULL;
    }

    // Update the number of bytes read
    bytes_read += rc;
  }

  result[len] = '\0';

  return result;
}

// Run this function for every cliet
void* client_fn(void* arg) {
  
  int socket_fd = *(int*)arg;

  while(1) {
    // Get a concatenated message
    char* long_message = receive_message(socket_fd);
    char* message = long_message; 

    // If no message is found
    if (message == NULL) {
      delete_neighbor(socket_fd);
      return NULL;
    }

    // Loop through each neighbor and send message except for the sender himself
    for (int i = 0; i < numNeighbors; i++){
      if(neighbors[i] == socket_fd){
        continue;
      }
      send_message(neighbors[i], message);
    }

  // Split the message into username and message
    int i;
    for (i = 0; i < 50; i++) {
      if (long_message[i] == ':') {
        long_message[i] = '\0';
        break;
      }
    }

    char* peer_username = long_message;
    pthread_mutex_t mutex2;
    // If the receiver is set to true, read the display the decrypted message
    if(isReceiver){
      pthread_mutex_lock(&mutex2);
      char buffer2[6];
      char message2[100];
      int fd = open("secret.txt", O_RDWR | O_CREAT);
      // Reads the key from the sender
      read(fd, buffer2, strlen(keys));
      // Display the decrypted message using the key read above.
      ui_display(peer_username, originalText(message2,message + i + 1, buffer2));
      pthread_mutex_unlock(&mutex2);
    }
    // Else, read the encrypted message
    else{
      ui_display(peer_username, message + i + 1);
    }
  }
  return NULL;
}

int main(int argc, char** argv) {

  // Keys is unique for each user.
  gen_random(keys);
  
  // Set up the user interface. The input_callback function will be called each time the user hits enter
  ui_init(input_callback);
  username = argv[1];

  // Set up a server socket to accept incoming connections
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(EXIT_FAILURE);
  }

  // Start listening for connections, with a maximum of one queued connection
  if (listen(server_socket_fd, 1)) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }

  // Did the user specify a peer we should connect to?
  if (argc == 4) {

    // Unpack arguments
    char* peer_hostname = argv[2];
    unsigned short peer_port = atoi(argv[3]);

    // Connect to another peer in the chat network
    int socket_fd = socket_connect(peer_hostname, peer_port);
    if (socket_fd == -1) {
      perror("Failed to connect");
      exit(EXIT_FAILURE);
    }

    add_neighbor(socket_fd);

    // Create a thread to send messages
    pthread_t client_thread;
    if(pthread_create(&client_thread, NULL, client_fn, &socket_fd) != 0){
      perror("pthread_create failed");
      exit(EXIT_FAILURE);
    }
  }

  pthread_t server_thread;
  if(pthread_create(&server_thread, NULL, server_fn, &server_socket_fd) != 0){
    perror("pthread_create failed");
    exit(EXIT_FAILURE);
  }

  // Once the UI is running, you can use it to display log messages
  char buffer[100];
  snprintf(buffer, 100, "listening for connections in port %u\n", port);
  ui_display("INFO", buffer);
  // Run the UI loop. This function only returns once we call ui_stop() somewhere in the program.
  ui_run();

  return 0;
}