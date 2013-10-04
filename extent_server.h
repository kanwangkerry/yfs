// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"

class extent_server {

 public:
  extent_server();

  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
  int get_dir(extent_protocol::extentid_t id, std::string name, extent_protocol::extentid_t &file_id);
  int put_dir(extent_protocol::extentid_t id, std::string name, extent_protocol::extentid_t &file_id);
  int read_dir_name(extent_protocol::extentid_t id, std::string &dir_name);
  int read_dir_id(extent_protocol::extentid_t id, std::string &dir_id);
  int setattr(extent_protocol::extentid_t id, extent_protocol::attr attr, int &);

  
 private:
  pthread_mutex_t extent_server_m;

  std::map<extent_protocol::extentid_t, std::string> content;
  std::map<extent_protocol::extentid_t, extent_protocol::attr> attribute;
  std::map<extent_protocol::extentid_t, std::map<std::string, extent_protocol::extentid_t> > dir_content;

};

#endif 







