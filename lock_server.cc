// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r){
	ScopedLock ml(&m_lock);

	if(locks.count(lid) == 0)
		locks.insert(lid, lock_server::FREE);

	while(locks[lid] != lock_status::FREE){
		pthread_cond_wait(&lock_assigned, &m_lock);
	}


	/*
	lock_protocol::status ret = lock_protocol::OK;
	nacquire += 1;
	printf("acquire call from client %d\n on lock id %llu.", clt, lid);
	r = nacquire;
	return ret;
	*/
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r){
	lock_protocol::status ret = lock_protocol::OK;
	nacquire -= 1;
	printf("release call from client %d\n on lock id %llu.", clt, lid);
	r = nacquire;
	return ret;
}
