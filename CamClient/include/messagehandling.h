#ifndef __MESSAGEHANDLING_H
#define __MESSAGEHANDLING_H

void handlePing(PingMessage *msg, SOCKET sock);
void handleTakePicture(TakePictureMessage *msg, SOCKET sock);
void handleSendlog(SendlogMessage *msg, SOCKET sock);
void handleJSON(char *json, SOCKET sock);

#endif
