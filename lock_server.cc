// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
	pthread_mutex_init(&lock_server_acq_m, NULL);
	pthread_mutex_init(&lock_server_rel_m, NULL);
	pthread_cond_init (&lock_free_cond, NULL);
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
	//TODO: Is this correct if I keep lock the mutex when enter each function?
	//If I do so, multi-thread does not makes any sense, cause this single server thread will be waiting here and no other server thread can be called.
	//but, I actually need this because map is not thread safe.
	//So I think this should be correct.
	pthread_mutex_lock(&lock_server_acq_m);

	lock_protocol::status ret = lock_protocol::OK;
	if(locks.count(lid) == 0)
		locks.insert(std::pair<lock_protocol::lockid_t, lock_server::lock_status>(lid, lock_server::FREE));

	while(locks[lid] != lock_server::FREE){
		pthread_cond_wait(&lock_free_cond, &lock_server_acq_m);
	}

	locks[lid] = lock_server::LOCKED;
	printf("acuqire call from client %d on lock id %llu.\n", clt, lid);
	nacquire += 1;

	r = 0;

	pthread_mutex_unlock(&lock_server_acq_m);

	return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r){
	pthread_mutex_lock(&lock_server_rel_m);

	lock_protocol::status ret = lock_protocol::OK;

	if(locks[lid] == lock_server::FREE)
		ret = lock_protocol::IOERR;
	else
		printf("release call from client %d on lock id %llu.\n", clt, lid);


	locks[lid] = lock_server::FREE;
	pthread_cond_broadcast(&lock_free_cond);

	nacquire -= 1;
	r = nacquire;

	pthread_mutex_unlock(&lock_server_rel_m);
	if(ret == lock_protocol::OK){
		printf("OKOK!!");
	}
	return ret;
}
