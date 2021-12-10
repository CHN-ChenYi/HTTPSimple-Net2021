#include "HTTPSimple.hpp"
#include "XForm.hpp"

void test(const HttpRequestPtr&&,
          std::function<void(const HttpResponsePtr&,
                             const HttpStatusCode& status_code)>&& callback) {
  HttpResponsePtr resp = std::make_unique<HttpResponse>();
  resp->SetContentType("text/html");
  resp->SetBody("./example/test.html");
  callback(resp, HttpStatusCode::OK);
};

void noimg(const HttpRequestPtr&&,
           std::function<void(const HttpResponsePtr&,
                              const HttpStatusCode& status_code)>&& callback) {
  HttpResponsePtr resp = std::make_unique<HttpResponse>();
  resp->SetContentType("text/html");
  resp->SetBody("./example/noimg.html");
  callback(resp, HttpStatusCode::OK);
};

void dopost(const HttpRequestPtr&& req,
            std::function<void(const HttpResponsePtr&,
                               const HttpStatusCode& status_code)>&& callback) {
  HttpResponsePtr resp = std::make_unique<HttpResponse>();
  if (req->headers["Content-Type"] != "application/x-www-form-urlencoded") {
    HttpResponsePtr resp = std::make_unique<HttpResponse>();
    resp->SetContentType("text/html");
    const std::string s("Unsupported content type!");
    resp->SetBody(std::move(s), s.size());
    callback(resp, HttpStatusCode::OK);
  }
  auto form = DecodeXWWWFormUrlencoded(req->body);
  if (form["login"] == "3190104500" &&
      form["pass"] == "4500") {  // correct password
    HttpResponsePtr resp = std::make_unique<HttpResponse>();
    resp->SetContentType("text/html");
    const std::string s("Login Success!");
    resp->SetBody(std::move(s), s.size());
    callback(resp, HttpStatusCode::OK);
  } else {  // incorrect password
    HttpResponsePtr resp = std::make_unique<HttpResponse>();
    resp->SetContentType("text/html");
    const std::string s("Login Failed!");
    resp->SetBody(std::move(s), s.size());
    callback(resp, HttpStatusCode::UNAUTHORIZED);
  }
};

void img(const HttpRequestPtr&&,
         std::function<void(const HttpResponsePtr&,
                            const HttpStatusCode& status_code)>&& callback) {
  HttpResponsePtr resp = std::make_unique<HttpResponse>();
  resp->SetContentType("image/jpeg");
  resp->SetBody("./example/logo.jpg");
  callback(resp, HttpStatusCode::OK);
};
