#include "tcp_socket.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <string>
#include <future>
#include <list>
#include <vector>
#include <sstream>
#include <map>
#include <functional>
#include <random>
#include <iostream>

using namespace ipsockets;
using namespace std::chrono_literals;

#if true
static const ip_type_e cfg_ip_type = v4;
static const addr4_t   cfg_server  = "127.0.0.1:8080";
#else
static const ip_type_e cfg_ip_type = v6;
static const addr6_t   cfg_server  = "[::1]:8080";
#endif

// ============================================================
// Demo HTTP server for ip-sockets-cpp-lite
// ============================================================

template <ip_type_e Ip_type>
struct mini_http_server_t {

  typedef tcp_socket_t<Ip_type, socket_type_e::server> tcp_server_t;
  typedef tcp_socket_t<Ip_type, socket_type_e::client> tcp_client_t;
  typedef addr_t<Ip_type>                              address_t;

  std::thread  worker_thread;
  bool         must_die;

  address_t    server_addr;
  tcp_server_t server_socket;

  std::map<std::string, std::function<std::string()> > routes;

  std::chrono::steady_clock::time_point start_time;
  uint64_t                        total_requests;
  std::map<std::string, uint64_t> route_count;

  // ===== constructor / destructor =====

  mini_http_server_t(address_t addr)
    : must_die(false), server_addr(addr), server_socket(log_e::info), total_requests(0) {
    start_time = std::chrono::steady_clock::now();
    setup_routes();
    start();
  }

  ~mini_http_server_t() { stop(); }

  // ===== lifecycle =====

  void start() {
    worker_thread = std::thread(&mini_http_server_t::worker, this);
  }

  void stop() {
    must_die = true;
    server_socket.close();
    if (worker_thread.joinable()) worker_thread.join();
  }

  // ===== worker loop =====

  void worker() {

    while (!must_die) {

      if (server_socket.open(server_addr) != no_error) { std::this_thread::sleep_for(1s); continue; }

      std::cout << "Server started on " << server_addr << std::endl;

      // List of tasks for handling clients
      std::list<std::future<void> > tasks;
      address_t                     client_addr;

      // Main accept loop
      while (!must_die && server_socket.state == state_e::state_opened) {

        tcp_client_t client = server_socket.accept(client_addr);
        if (client.state != state_e::state_opened) continue;

        // Handle client in separate thread
        tasks.push_back(std::async(std::launch::async,&mini_http_server_t::handle_client,this,std::move(client),client_addr));

        // Clean up completed tasks
        std::list<std::future<void> >::iterator it = tasks.begin();
        while (it != tasks.end()) { if (it->wait_for(0s) == std::future_status::ready) it = tasks.erase(it); else ++it; }
      }

      // Wait for all tasks to complete
      std::list<std::future<void> >::iterator it = tasks.begin();
      while (it != tasks.end()) { it->wait(); ++it; }
    }
  }

  // ===== request handling =====

  void handle_client (tcp_client_t client, address_t addr) {

    // Buffer for reading the request
    std::vector<char> buffer(4096);

    // Read request from client
    int bytes = client.recv (&buffer[0], (int)buffer.size());
    if (bytes <= 0) { client.close(); return; }

    // Parse the request line (simple parsing)
    std::string request (&buffer[0], (size_t)bytes);
    std::istringstream ss (request);
    std::string method, path, version;
    ss >> method >> path >> version;

    // Redirect empty path to "/"
    if (path.empty()) path = "/";

    // Increment counters
    total_requests++;
    route_count[path]++;

    // Prepare response
    std::string type        = "text/html; charset=utf-8";
    int         status      = 200;
    std::string status_text = "OK";
    std::string body;

    // Find matching route
    std::map<std::string,std::function<std::string()> >::iterator it = routes.find(path);
    if (it != routes.end()) {
      body = it->second(); // Call the handler of found route
      // fix content type for some routes, witch return non-HTML content
      if (path == "/api/status")  type = "application/json";
      if (path == "/favicon.ico") type = "image/png";
      if (path == "/favicon.svg") type = "image/svg+xml";
      std::cout << "Request: " << method << " " << path << " type: " << type << std::endl;
    }
    else { // Requested route not found
      status = 404; status_text = "Not Found";
      body   = "<h1>404 Not Found</h1><p>Page does not exist.</p><a href='/'>← Home</a>";
    }

    // Send HTTP response
    send_response (client, status, status_text, type,body);
    client.close();
    std::cout << "Closed connection " << addr << std::endl;
  }

  // ===== HTTP response =====

  void send_response (tcp_client_t &client,int status,const std::string &status_text,const std::string &type,const std::string &body) {

    std::ostringstream headers;
    // Status line
    headers << "HTTP/1.1 "        << status      << " " << status_text << "\r\n";
    // Headers
    headers << "Content-Type: "   << type        << "\r\n";
    headers << "Content-Length: " << body.size() << "\r\n";
    headers << "Cache-Control: public, max-age=86400\r\n";
    headers << "Connection: close\r\n";
    headers << "Server: mini-http-server\r\n";
    // Date header
    std::time_t now = std::time (NULL);
    headers << "Date: " << std::put_time (gmtime (&now), "%a, %d %b %Y %H:%M:%S %Z") << "\r\n";
    // Empty line to separate headers from body
    headers << "\r\n";

    // Send headers
    std::string h = headers.str();
    client.send (h.data(), (int)h.size());
    // Send body (if it not empty)
    if (!body.empty())
      client.send (body.data(), (int)body.size());
    std::cout << "Response: " << status << " " << status_text << " (" << body.size() << " bytes)" << std::endl;
  }

  // ===== uptime =====

  std::string uptime() const {
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds> (std::chrono::steady_clock::now() - start_time);
    uint64_t h = (uint64_t)s.count()/3600, m = ((uint64_t)s.count()%3600)/60, sec = (uint64_t)s.count()%60;
    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << h << ":" << std::setw(2) << m << ":" << std::setw(2) << sec;
    return os.str();
  }

  // ===== routes =====

  void setup_routes() {
    routes["/"]            = std::bind(&mini_http_server_t::page_home,   this);
    routes["/about"]       = std::bind(&mini_http_server_t::page_about,  this);
    routes["/time"]        = std::bind(&mini_http_server_t::page_time,   this);
    routes["/random"]      = std::bind(&mini_http_server_t::page_random, this);
    routes["/stats"]       = std::bind(&mini_http_server_t::page_stats,  this);
    routes["/api/status"]  = std::bind(&mini_http_server_t::api_status,  this);
    routes["/favicon.ico"] = std::bind(&mini_http_server_t::favicon_ico, this);
    routes["/favicon.svg"] = std::bind(&mini_http_server_t::favicon_svg, this);
  }

  // ===== favicon (valid PNG binary served as favicon.ico) =====

  std::string favicon_ico() {
    static const unsigned char png[] = {
      137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,16,0,0,0,16,8,6,0,0,0,31,243,255,97,
      0,0,0,21,73,68,65,84,56,203,99,252,255,159,1,12,12,12,12,0,0,13,131,2,95,111,37,47,0,0,0,
      0,73,69,78,68,174,66,96,130
    };
    return std::string((const char*)png,sizeof(png));
  }

  std::string favicon_svg() {
    return R"(
      <?xml version="1.0" encoding="UTF-8"?>
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
        <rect width="100" height="100" rx="20" fill="#111"/>
        <text x="50%" y="55%" dominant-baseline="middle" text-anchor="middle" font-size="48" fill="#fff" font-family="Arial">IP</text>
      </svg>
    )";
  }

  // ===== pages =====

  std::string page_home() {
    return
      "<!DOCTYPE html>"
      "<html>"
        "<head>"
          "<title>Mini HTTP Server</title>"
          "<style>body{font-family:Arial;margin:40px;line-height:1.6;}</style>"
          //"<link rel='icon' href='/favicon.ico'>"
          "<link rel='icon' type='image/svg+xml' href='/favicon.svg'>"
        "</head>"
        "<body>"
          "<h1>🚀 Mini HTTP Server</h1>"
          "<p>Welcome to your lightweight C++ HTTP server!</p>"
          "<p>Built with <a href='https://github.com/biaks/ip-sockets-cpp-lite'>ip-sockets-cpp-lite</a>.</p>"
          "<h2>Available pages:</h2>"
          "<ul>"
            "<li><a href='/'>Home</a></li>"
            "<li><a href='/about'>About</a></li>"
            "<li><a href='/time'>Current Time</a></li>"
            "<li><a href='/random'>Random Number</a></li>"
            "<li><a href='/stats'>Server Stats</a></li>"
            "<li><a href='/api/status'>API Status</a></li>"
          "</ul>"
        "</body>"
      "</html>";
  }

  std::string page_about() {
    return
      "<!DOCTYPE html>"
      "<html>"
        "<head>"
          "<title>About</title>"
          "<style>body{font-family:Arial;margin:40px;}</style>"
        "</head>"
        "<body>"
          "<h1>📖 About</h1>"
          "<p>Simple demo HTTP server built with <a href='https://github.com/biaks/ip-sockets-cpp-lite'>ip-sockets-cpp-lite</a>.</p>"
          "<a href='/'>← Back</a>"
        "</body>"
      "</html>";
  }

  std::string page_time() {
    std::time_t t=std::time(NULL); std::ostringstream os;
    os <<
      "<!DOCTYPE html>"
      "<html>"
        "<head>"
          "<title>Time</title>"
          "<style>body{font-family:Arial;margin:40px;}</style>"
          "<meta http-equiv='refresh' content='5'>"
        "</head>"
        "<body>"
          "<h1>⏰ Current Time</h1>"
          "<p style='font-size:24px;font-weight:bold;'>"<<std::put_time(std::gmtime(&t),"%Y-%m-%d %H:%M:%S UTC")<<"</p>"
          "<a href='/'>← Back</a>"
        "</body>"
      "</html>";
    return os.str();
  }

  std::string page_random() {
    static std::mt19937 gen((unsigned)std::time(NULL));
    static std::uniform_int_distribution<> dist(1,1000);
    std::ostringstream os;
    os <<
      "<!DOCTYPE html>"
      "<html>"
        "<head>"
          "<title>Random</title>"
          "<style>body{font-family:Arial;margin:40px;text-align:center;}</style>"
        "</head>"
        "<body>"
          "<h1>🎲 Random Number</h1>"
          "<p style='font-size:48px;font-weight:bold;color:#4CAF50;'>"<<dist(gen)<<"</p>"
          "<a href='/random'>Generate another →</a><br>"
          "<a href='/'>← Back</a>"
        "</body>"
      "</html>";
    return os.str();
  }

  std::string page_stats() {
    std::ostringstream os;
    os <<
      "<!DOCTYPE html>"
      "<html>"
        "<head>"
          "<title>Stats</title>"
          "<style>body{font-family:Arial;margin:40px;}</style>"
        "</head>"
        "<body>"
          "<h1>📊 Server Statistics</h1>"
          "<p>Total requests: "<<total_requests<<"</p>"
          "<p>Uptime: "<<uptime()<<"</p>"
          "<a href='/'>← Back</a>"
        "</body>"
      "</html>";
    return os.str();
  }

  std::string api_status() {
    std::ostringstream os;
    os <<
      "{ \"status\":\"ok\", "
        "\"requests\": "<<total_requests<<", "
        "\"uptime\":\""<<uptime()<<"\" "
      "}";
    return os.str();
  }

};

// ===== main =====

int main() {

  mini_http_server_t<cfg_ip_type> server(cfg_server);

  std::cout << "\nMini HTTP Server is running!\n";
  std::cout << "Open your browser and visit:\n";
  std::cout << "   http://" << cfg_server << "/\n";
  std::cout << "\nAvailable routes:\n";
  std::cout << "   /           - Home page\n";
  std::cout << "   /about      - About page\n";
  std::cout << "   /time       - Current server time\n";
  std::cout << "   /random     - Random number generator\n";
  std::cout << "   /stats      - Server statistics\n";
  std::cout << "   /api/status - JSON API status\n";
  std::cout << "\nPress Ctrl+C to stop the server...\n\n";

  while (true) std::this_thread::sleep_for(1s);

  return 0;
}
