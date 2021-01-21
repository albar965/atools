/**
 *  @file
 *  @author Stefan Frings
 */

#include "httprequesthandler.h"

using namespace stefanfrings;

HttpRequestHandler::HttpRequestHandler(QObject *parent)
  : QObject(parent)
{
}

void HttpRequestHandler::service(HttpRequest& request, HttpResponse& response)
{
  qCritical("HttpRequestHandler: you need to override the service() function");
  qDebug("HttpRequestHandler: request=%s %s %s", request.getMethod().constData(),
         request.getPath().constData(), request.getVersion().constData());
  response.setStatus(501, "not implemented");
  response.write("501 not implemented", true);
}
