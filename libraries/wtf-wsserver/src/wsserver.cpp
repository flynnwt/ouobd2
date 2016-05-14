#include "wsserver.h"

WebsocketServer::WebsocketServer(unsigned int port) {
  this->port = port;
  server = new WiFiServer(port);
  socket = new WebSocketServer();
}

void WebsocketServer::setConnect(void (cb)()) {
  cbConnect = cb;
}

void WebsocketServer::setDisconnect(void (cb)()) {
  cbDisconnect = cb;
}

void WebsocketServer::setReceive(void (cb)(String data)) {
  cbReceive = cb;
}

void WebsocketServer::begin() {
  server->begin();
}

void WebsocketServer::process() {
    String data;

    if (!connected) {
      client = server->available();
      if (socket->handshake(client)) {
        connected = true;
        if (cbConnect != NULL) {
          (cbConnect)();
        }
      }
    }

    if (connected) {
      if (client.connected()) {   
        data = socket->getData();
        if (data.length() > 0) {
          if (cbReceive != NULL) {
            (cbReceive)(data);
          }
        }

      } else {
        // the close() doesn't happen until all the queued up messages are sent, i guess; in browser, see closing is started,
        //  but close event doesn't occur until server finishes sending; this is with 100 byte messages, so there may be a lag
        //  all the time if sending too fast for ESP to handle - but it ran a long time (>10000 messages), so there isn't an
        //  overflow ever
        // 100B: hit disconnect on message 19, server sent 117
        //  10B:                           31, server sent 129
        // so it appears to be related to server comm speed, not message size
        connected = false;
        if (cbDisconnect != NULL) {
          (cbDisconnect)();
        }
      }
    }

}

void WebsocketServer::send(String data) {
  socket->sendData(data);
}
