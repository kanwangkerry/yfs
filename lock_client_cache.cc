// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


lock_client_cache::lock_client_cache(std::string xdst, 
		class lock_release_user *_lu)
: lock_client(xdst), lu(_lu)
{
	rpcs *rlsrpc = new rpcs(0);
	rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
	rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

	const char *hname;
	hname = "127.0.0.1";
	std::ostringstream host;
	host << hname << ":" << rlsrpc->port();
	id = host.str();
	pthread_mutex_init(&_m, NULL);
}

	lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
	//tprintf("%s acquire lid: %lld\n", id.c_str(), lid);
	pthread_mutex_lock(&_m);
	int ret = lock_protocol::OK;
	// lock is not valid in the client side.
	if(lock_state_map.count(lid) == 0){
		lock_state_map[lid] = lock_client_cache::none;
		lock_queue[lid] = 0;
		pthread_cond_t *temp;
		temp = new pthread_cond_t;
		pthread_cond_init(temp, NULL);
		lock_cond[lid] = *temp;
		temp = new pthread_cond_t;
		pthread_cond_init(temp, NULL);
		lock_revoke[lid] = *temp;
		temp = new pthread_cond_t;
		pthread_cond_init(temp, NULL);
		lock_revoke_finished[lid] = *temp;
		lock_revoke_called[lid] = false;
		temp = new pthread_cond_t;
		pthread_cond_init(temp, NULL);
		lock_retry[lid] = *temp;
		lock_retry_called[lid] = false;
	}

	int r =0;
	while(1){
		switch(lock_state_map[lid]){
			case lock_client_cache::none:
				//none: use rpc to acquire the lock.
				//remeber to check the return value:
				//ok means rpc is correct and lock it got
				//RETRY means RPC not currently available: change the state to acquiring, sleep for a while, then acquire again.
				lock_state_map[lid] = lock_client_cache::acquiring;

				pthread_mutex_unlock(&_m);
				ret = cl->call(lock_protocol::acquire, lid,id, r);

				pthread_mutex_lock(&_m);
				if(ret == lock_protocol::OK){
					//acquire is ok.
					//TODO: take care of revoked
					if(lock_state_map[lid] == lock_client_cache::acquiring){
						lock_state_map[lid] = lock_client_cache::locked;
						pthread_mutex_unlock(&_m);
						return ret; 
					}
					else{
						//tprintf("state is not correct: should be acquiring");
					}
				}
				else if (ret == lock_protocol::RETRY){
					if(lock_retry_called[lid])
						lock_retry_called[lid] = false;
					else{
							//tprintf("start waiting on client\n");
						while(!lock_retry_called[lid])
						{
							//tprintf("waiting on client");
							pthread_cond_wait(&lock_retry[lid], &_m);
						}
						}
					//retry is called correctly.
					//so this thread have got the owership
					lock_retry_called[lid] = false;
					lock_state_map[lid] = locked;
					pthread_mutex_unlock(&_m);
					return lock_protocol::OK;
				}
				else{
					pthread_mutex_unlock(&_m);
					return ret;
				}
				break;
			case lock_client_cache::free:
				//free state: lock is free on this clean. assign it to the thread then change state to locked;
				lock_state_map[lid] = locked;
				pthread_mutex_unlock(&_m);
				return lock_protocol::OK;
			case lock_client_cache::locked:
				//locked state: lock is being used by some other threads. wait on the signal.
				lock_queue[lid]++;
				while(lock_state_map[lid] != lock_client_cache::free){
					pthread_cond_wait(&lock_cond[lid], &_m);
				}
				lock_queue[lid]--;
				lock_state_map[lid] = lock_client_cache::locked;
				pthread_mutex_unlock(&_m);
				return lock_protocol::OK;
			case lock_client_cache::acquiring:
				//This state means, another thread is requesting the lock. Just wait on the signal until the requesting lock free it.
				lock_queue[lid]++;
				while(lock_state_map[lid] != lock_client_cache::free){
					pthread_cond_wait(&lock_cond[lid], &_m);
				}
				lock_queue[lid]--;
				lock_state_map[lid] = lock_client_cache::locked;
				pthread_mutex_unlock(&_m);
				return lock_protocol::OK;
			case lock_client_cache::releasing:
				//revoke is called, so the current thread is or will release the lock to lock server. Put the thread into the queue, and wait for signal.
				//In revoke: after the release RPC call, check if there is another thread waiting on this lock. If so, call acquire and do as the none state do. send signal when get the lock.
				//in new method: wait for a release call finish.

				while(lock_state_map[lid] == lock_client_cache::releasing)
					pthread_cond_wait(&lock_revoke_finished[lid], &_m);
				break;
			default:
				tprintf("Lock state wrong, lid: %llu\n", lid);

		}
	}

	pthread_mutex_unlock(&_m);
	return lock_protocol::IOERR;
}

	lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
	//tprintf("%s release lid: %lld\n", id.c_str(), lid);
	//when release, need to check the state of lock.
	//if it is not releasing, revoke is not called, so just signal the waiting ones.
	//if it is releasing, revoke is called, so call release to give back the ownership.
	//after releasing the ownership, check if there is threads waiting in the queue. If so, acquire the ownership again and reduce the number of threads waiting in the queue after get the ownership.
	//do not do this ask again!
	//just wait until no one need lock here
	int ret;
	pthread_mutex_lock(&_m);
	lock_state_map[lid] = lock_client_cache::free;
	if(!lock_revoke_called[lid] || lock_queue[lid] >0){
		pthread_cond_broadcast(&lock_cond[lid]);
		pthread_mutex_unlock(&_m);
		return lock_protocol::OK;
	}
	else{
		// release the lock
		int r = 0;
		lock_revoke_called[lid] = false;
		lock_state_map[lid] = releasing;
		pthread_mutex_unlock(&_m);
		//tprintf("%s revoke lid: %lld calling release\n", id.c_str(), lid);
		ret = cl->call(lock_protocol::release, lid, id, r);
		VERIFY (ret == lock_protocol::OK); 
		pthread_mutex_lock(&_m);
		lock_state_map[lid] = lock_client_cache::none;
		pthread_cond_broadcast(&lock_revoke_finished[lid]);
		pthread_mutex_unlock(&_m);
		return lock_protocol::OK;
	}

}

	rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
		int &)
{
	//revoke means get back the ownership. if it is free, release it immediately. otherwise, set the state to releasing and wait on the condition.
	//I need 2 signal: one for releas to other threads, one for release to the server. Each time when release, if realing is set, then just release to the server.
	//to solve problem with recieving revoke before getting OK: check the state ofthe lock, if it is acquiring then change to releasing. acquire handler will check if releasing, then release it directly.  
	//if client got OK but need to release it immediately, increment the number of threads waiting on this lock. Then after the release it will be acquired again.
	//This methods means toooooo much logic on revoke/release.
	//I come up with a better idea: hold the lock until no threads waiting on this client!
	//This makes sense and makes everything easier.
	//tprintf("%s revoke lid: %lld\n", id.c_str(), lid);
	int ret = rlock_protocol::OK;
	int r = 0;
	pthread_mutex_lock(&_m);
	lock_revoke_called[lid] = true;
	if(lock_state_map[lid] != lock_client_cache::free || lock_queue[lid] > 0){
		pthread_mutex_unlock(&_m);
		return rlock_protocol::OK;
	}
	//reach here means I could release the ownership to the server directly
	lock_state_map[lid] = releasing;
	pthread_mutex_unlock(&_m);
	//tprintf("%s revoke lid: %lld calling release\n", id.c_str(), lid);
	ret = cl->call(lock_protocol::release, lid, id, r);
	VERIFY (ret == lock_protocol::OK); 
	pthread_mutex_lock(&_m);
	lock_state_map[lid] = lock_client_cache::none;
	pthread_cond_broadcast(&lock_revoke_finished[lid]);
	pthread_mutex_unlock(&_m);
	return rlock_protocol::OK;
}

	rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
		int &)
{
	//tprintf("%s retry lid: %lld\n", id.c_str(), lid);
	int ret = rlock_protocol::OK;
	pthread_mutex_lock(&_m);
	lock_retry_called[lid] = true;
	pthread_cond_broadcast(&lock_retry[lid]);
	pthread_mutex_unlock(&_m);
	return ret;
}



