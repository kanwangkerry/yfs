// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
 public:
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache() {};
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
                                       int &);
  enum lock_status{
	  none = 0x8001,
	  free,
	  locked,
	  acquiring,
	  releasing
  };
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;

  std::map<lock_protocol::lockid_t, lock_client_cache::lock_status> lock_state_map;
  //maintain how many threads is waiting on the lock
  std::map<lock_protocol::lockid_t, int> lock_queue;
  //maintain a cond signal for each lock
  std::map<lock_protocol::lockid_t, pthread_cond_t> lock_cond;
  //maintain another cond for revoking
  std::map<lock_protocol::lockid_t, pthread_cond_t> lock_revoke;
  std::map<lock_protocol::lockid_t, bool> lock_revoke_called;
  std::map<lock_protocol::lockid_t, pthread_cond_t> lock_revoke_finished;
  //maintain a cond on retry
  //retry means the server directly send the ownership to this client.
  //do not need to send acquire again
  std::map<lock_protocol::lockid_t, pthread_cond_t> lock_retry;

  std::map<lock_protocol::lockid_t, bool> lock_retry_called;
  pthread_mutex_t _m;

};


#endif
