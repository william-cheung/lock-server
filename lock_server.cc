// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
  VERIFY(pthread_mutex_init(&mutex, NULL) == 0);
  VERIFY(pthread_cond_init(&cond, NULL) == 0);
}

lock_server::~lock_server() 
{
  VERIFY(pthread_mutex_destroy(&mutex) == 0);
  VERIFY(pthread_cond_destroy(&cond) == 0);
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
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &) 
{
  pthread_mutex_lock(&mutex);

  while (state_map.find(lid) != state_map.end() 
         && state_map[lid] == state_locked) {
    //printf("lock_server: clt %010d WAIT %016llx\n", clt, lid);
    pthread_cond_wait(&cond, &mutex);
  }

  state_map[lid] = state_locked;
  own_map[lid] = clt; 
  //printf("lock_server: clt %010d HOLD %016llx\n", clt, lid);
  pthread_mutex_unlock(&mutex);
  return lock_protocol::OK;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &)
{
  
  pthread_mutex_lock(&mutex);
  if (state_map.find(lid) != state_map.end()
      && state_map[lid] == state_locked 
      && own_map[lid] == clt) {
    state_map[lid] = state_free;
    own_map.erase(lid);
    pthread_cond_signal(&cond);
    //printf("lock_server: clt %010d FREE %016llx\n", clt, lid);
  }
  pthread_mutex_unlock(&mutex);

  return lock_protocol::OK;
}
