// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <map>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

class lock_server {

	protected:
		int nacquire;
		enum lock_status {LOCKED, FREE};
		std::map<lock_protocol::lockid_t, lock_server::lock_status> locks;

		//2 mutex for acquire and release
		pthread_mutex_t lock_server_acq_m;
		pthread_mutex_t lock_server_rel_m;
		pthread_cond_t lock_free_cond;

	public:
		lock_server();
		~lock_server() {};
		lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
		lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
		lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);

};

#endif 







