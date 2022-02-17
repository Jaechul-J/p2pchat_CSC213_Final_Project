#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "socket.h"
#include "ui.h"

int neighbors[5] = {1,2,3,4,5};
int size = 5;

void delete_neighbor(int socket){
  int index = 0;
  while(neighbors[index] != socket) {
    index++;
  }
  int i;
  for (i = index; i < size - 1; i++){
    neighbors[i] = neighbors[i+1];
  }
  neighbors[i] = -1;
  size--;
}

int main() {
    
    delete_neighbor(5);

    for (int i=0;i<size;i++) {
        printf("%d ",neighbors[i]);
    }
    printf("\n");
}