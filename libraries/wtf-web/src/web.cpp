#include "web.h"

Web* gWeb;
ESP8266WebServer* gServer;

// can't use methods directly because of implied 'this'
void handleRoot() {
  String message = "Not configured";
  gServer->send(200, "text/plain", message);
}

void handleNotFound() {
  String message = "File Not Found";
  gServer->send(404, "text/plain", message);
}

void handleUpload() {
  gWeb->uploader();  
}

Web::Web() {
  init(80);   
}

Web::Web(unsigned int port) {
  init(port);
}

void Web::init(unsigned int port) {
  gWeb = this;
  server = new ESP8266WebServer(port);
  gServer = server;
  cbUploadComplete = NULL;
  // THIS ISN'T REPLACED in addRoot()!!!!!
  // would have to access current list and delete/update
  //server->on("/", handleRoot);  
  //server->on("/index.html", handleRoot);
  //server->onNotFound(handleNotFound);
  server->begin();
}

void Web::addRoot(void(handler)()) {
  server->on("/", handler);
  server->on("/index.html", handler);
};

void Web::addRoute(String route, void (handler)()) {
  server->on(route.c_str(), handler);
}

void Web::addStatic(String uri, String path) {
  SPIFFS.begin();
  server->serveStatic(uri.c_str(), SPIFFS, path.c_str());
}

void Web::addUpload(String uri) {
  addUpload(uri, handleUpload);
}

void Web::addUpload(String uri, void (cb)()) {
  uploadUri = uri;
  server->on(uri.c_str(), HTTP_POST, [&]() { // whoa - anon function in cpp
    server->send(200, "text/plain", "");
  });
  //called when a file is received inside POST data
  server->onFileUpload(cb);
}

void Web::addUploadComplete(void (cb)(String filename, int err)) {
  cbUploadComplete = cb;
}

// could also check open/writes/close for error and report in gServer->uploadError
// could use a temp file and only do a copy/delete at end if ok
void Web::uploader() {

  if (server->uri() != uploadUri) return;  // this is called regardless of location, so prevent bad POSTs by checking

  HTTPUpload& upload = server->upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    uploadFilename = filename;
    uploadError = 0;
    uploadFile = SPIFFS.open((char *)filename.c_str(), "w"); // looks like SPIFFS dont support b yet!!! see FS.cpp
  // if fail - mark error and call uploadcomplete if exists; fail = null; rest of calls will be ignored
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
     uploadFile.write(upload.buf, upload.currentSize);
      // if fail - mark error and call uploadcomplete if exists : fail = bytes written != currentsize
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      // if fail - mark error and call uploadcomplete if exists: fail = 
      uploadFile = (File)NULL;
    }
    if (cbUploadComplete) {
      (cbUploadComplete)(uploadFilename, uploadError);
    }
  }

}
