// Author: Alexander Thomson (thomson@cs.yale.edu)
// Author: Kun Ren (kun@cs.yale.edu)
//
// A simple implementation of the storage interface using an stl map.

#ifndef _DB_BACKEND_SIMPLE_STORAGE_H_
#define _DB_BACKEND_SIMPLE_STORAGE_H_

#include <tr1/unordered_map>

#include "backend/storage.h"
#include "common/types.h"
#include "common/utils.h"

using std::tr1::unordered_map;

class SimpleStorage : public Storage {
 public:
  virtual ~SimpleStorage() {}

  // TODO(Thad): Implement something real here
  virtual bool Prefetch(const Key &key, double* wait_time)  { return false; }
  virtual bool Unfetch(const Key &key)                      { return false; }
  virtual ValueStore* ReadObject(const Key& key, int64 txn_id = 0);
  virtual bool PutObject(const Key& key, ValueStore* value, int64 txn_id = 0);
  virtual bool DeleteObject(const Key& key, int64 txn_id = 0);

  virtual void PrepareForCheckpoint(int64 stable) {}
  virtual int Checkpoint() { return 0; }

 private:
  unordered_map<Key, ValueStore*> objects_;
};
#endif  // _DB_BACKEND_SIMPLE_STORAGE_H_

