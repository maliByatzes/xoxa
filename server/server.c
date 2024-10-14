/* server.c */

#include "server.h"

Client *newClient(SOCKET clientfd, char *hostname, char *ip_address, char *port)
{
  Client *this = (Client*)malloc(sizeof(Client));

  this->clientfd = clientfd;
  this->hostname = hostname;
  this->ip_address = ip_address;
  this->port = port;

  return this;
}

void deleteClient(Client *this)
{
  free(this);
}

NodeClient *newNodeClient(Client *data)
{
  NodeClient *newNC = (NodeClient*)malloc(sizeof(NodeClient));
  newNC->data = data;
  newNC->next = NULL;
  return newNC;
}

void insertAtFirst(NodeClient **head, Client *data)
{
  NodeClient *newNC = newNodeClient(data);
  newNC->next = *head;
  *head = newNC;
}

void insertAtEnd(NodeClient **head, Client *data)
{
  NodeClient *newNC = newNodeClient(data);
  if (*head == NULL) {
    *head = newNC;
    return;
  }

  NodeClient *temp = *head;
  while (temp->next != NULL)
    temp = temp->next;

  temp->next = newNC;
}

void insertAtPosition(NodeClient **head, Client *data, int position)
{
  NodeClient *newNC = newNodeClient(data);
  if (position == 0) {
    insertAtFirst(head, data);
    return;
  }

  NodeClient *temp = *head;
  for (int i = 0; temp != NULL && i < position; i++)
    temp = temp->next;

  if (temp == NULL) {
    fprintf(stderr, "Position out of range\n");
    free(newNC->data);
    free(newNC);
    return;
  }

  newNC->next = temp->next;
  temp->next = newNC;
}

void deleteFromFirst(NodeClient **head)
{
  if (*head == NULL) {
    printf("List is empty\n");
    return;
  }

  NodeClient *temp = *head;
  *head = temp->next;
  free(temp->data);
  free(temp);
}

void deleteFromEnd(NodeClient **head)
{
  if (*head == NULL) {
    printf("List is empty\n");
    return;
  }

  NodeClient *temp = *head;
  if (temp->next == NULL) {
    free(temp->data);
    free(temp);
    *head = NULL;
    return;
  }

  while (temp->next->next != NULL)
    temp = temp->next;

  free(temp->next->data);
  free(temp->next);

  temp->next = NULL;
}

void deleteAtPosition(NodeClient **head, int position)
{
  if (*head == NULL) {
    printf("List is empty\n");
    return;
  }

  NodeClient *temp = *head;
  if (position == 0) {
    deleteFromFirst(head);
    return;
  }

  for (int i = 0; temp != NULL && i < position; i++)
    temp = temp->next;

  if (temp == NULL || temp->next == NULL) {
    fprintf(stderr, "Position out of range\n");
    return;
  }

  NodeClient *next = temp->next->next;
  free(temp->next->data);
  free(temp->next);

  temp->next = next;
}
