// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>

lock_client::lock_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
  }
  pthread_mutex_init(&lock_client_acq_m, NULL);
  pthread_mutex_init(&lock_client_rel_m, NULL);
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
	pthread_mutex_lock(&lock_client_acq_m);
	//TODO: add multi-thread mutex
	int r;
	lock_protocol::status ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
	VERIFY (ret == lock_protocol::OK);
	pthread_mutex_unlock(&lock_client_acq_m);
	return ret;
}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
	//TODO: add multi-thread mutex
	pthread_mutex_lock(&lock_client_rel_m);
	int r;
	lock_protocol::status ret = cl->call(lock_protocol::release, cl->id(), lid, r);
	VERIFY (ret == lock_protocol::OK);
	pthread_mutex_unlock(&lock_client_rel_m);
	return ret;
}

