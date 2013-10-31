#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <map>
#include <queue>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"


class lock_server_cache {
	private:
		int nacquire;
		enum lock_status {LOCKED, FREE};
		std::map<lock_protocol::lockid_t, lock_server_cache::lock_status> locks;
		std::map<lock_protocol::lockid_t, std::queue<std::string> > lock_requests;
		std::map<lock_protocol::lockid_t, std::string> lock_owner;
		pthread_mutex_t lock_server_acq_m;
		pthread_mutex_t lock_server_rel_m;
		pthread_cond_t lock_free_cond;
	public:
		lock_server_cache();
		lock_protocol::status stat(lock_protocol::lockid_t, int &);
		int acquire(lock_protocol::lockid_t, std::string id, int &);
		int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
