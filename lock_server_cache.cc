// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache():
	nacquire (0)
{
	pthread_mutex_init(&lock_server_acq_m, NULL);
	pthread_mutex_init(&lock_server_rel_m, NULL);
	pthread_cond_init (&lock_free_cond, NULL);
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
		int &)
{
	//tprintf("acquire on server!, lid: %lld, client: %s\n", lid, id.c_str());
	pthread_mutex_lock(&lock_server_acq_m);
	lock_protocol::status ret = lock_protocol::OK;

	if(locks.count(lid) == 0){
		locks[lid] = lock_server_cache::FREE;
		lock_owner[lid] = "";
		lock_requests[lid] = std::queue<std::string>();
	}
	if(locks[lid] == FREE){
		locks[lid] = LOCKED;
		lock_owner[lid] = id;
	//tprintf("acquire on server! returning, lid: %lld, client: %s\n", lid, id.c_str());
	pthread_mutex_unlock(&lock_server_acq_m);
		return lock_protocol::OK;
	}

	//not free, have to revoke the ownership from the client.
	
	//not free, but more clients to wait, just put into queue.
	if(!lock_requests[lid].empty()){
		lock_requests[lid].push(id);
		pthread_mutex_unlock(&lock_server_acq_m);
		return lock_protocol::RETRY;
	}

	//not free, but no more free clients, just call the revoke 
	handle h(lock_owner[lid]);
	rpcc *cl = h.safebind();
	if(!cl){
		//tprintf("bind faile on %s\n", lock_owner[lid].c_str());
	}
	lock_requests[lid].push(id);
	int r;

	pthread_mutex_unlock(&lock_server_acq_m);
	ret = cl->call(rlock_protocol::revoke, lid, r);
	VERIFY(ret == rlock_protocol::OK);
	pthread_mutex_lock(&lock_server_acq_m);

	pthread_mutex_unlock(&lock_server_acq_m);

	return lock_protocol::RETRY;
}

	int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
		int &)
{
	//tprintf("release on server!");
	lock_protocol::status ret = lock_protocol::OK;
	//when released, we try to give the lock to the top of the queue, but also, we need to revoke it to make sure other threads waiting in the queue can get it.
	
	pthread_mutex_lock(&lock_server_acq_m);
	if(lock_requests[lid].empty()){
		//tprintf("No queue on lock: %lld\n", lid);
		locks[lid] = lock_server_cache::FREE;
	pthread_mutex_unlock(&lock_server_acq_m);
		return ret;
	}

	std::string owner_id = lock_requests[lid].front();
	lock_requests[lid].pop();
	lock_owner[lid] = owner_id;
	locks[lid] = LOCKED;
	handle h(owner_id);
	rpcc *cl = h.safebind();
	if(!cl){
		//tprintf("bind faile on %s\n", lock_owner[lid].c_str());
	}
	int r;

	pthread_mutex_unlock(&lock_server_acq_m);
	ret = cl->call(rlock_protocol::retry, lid, r);
	VERIFY(ret == rlock_protocol::OK);
	pthread_mutex_lock(&lock_server_acq_m);
	//tprintf("******retry in release called right");

	//till here, the lock has been given to the new owner in the queue
	//now we wait for some time and call revoke to make sure the clients in the queue will not wait forever
	//tprintf("***try to release from client %s\n", owner_id.c_str());
	if(!lock_requests[lid].empty()){
		pthread_mutex_unlock(&lock_server_acq_m);
		ret = cl->call(rlock_protocol::revoke, lid, r);
		VERIFY(ret == rlock_protocol::OK);
		pthread_mutex_lock(&lock_server_acq_m);
	//tprintf("******revoke in release called right");
	}
	pthread_mutex_unlock(&lock_server_acq_m);
	return lock_protocol::OK;
}

	lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
	//tprintf("stat request\n");
	r = nacquire;
	return lock_protocol::OK;
}

