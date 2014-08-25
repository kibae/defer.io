defer.io
========

JSON based document oriented DBMS

## Status
* Project status: Pre-Alpha
 * Current release was tested only Mac OS X.

## Feature
* JSON Document manipulation.
 * Global: get, getOrCreate, set, getSet, setIfNotExists
 * Array: push, pop, shift, unshift, splice, cut, slice, count, randGet, randPop
 * List: push, unshift, pushUniq, unshiftUniq
 * Object: set, setIfNotExists, del, keys
 * Number: incr, decr
 * String: append, prepend, length, sub
* High performance
 * Sharded Memory cache.
 * Document level exclusive-lock.
 * libev async IO.
 * leveldb (google) based storage engine.
 * Background lazy writer.
* Scale-out for high scalability
 * Multi slave real-time streaming replication (each touch, per virtual bucket)
 * Multi slave Lazy streaming replication (each checkout, per virtual bucket)
 * Streaming dump (per virtual bucket)
 * Managed live sharding (max 100 cluster)
* Async protocol

## Requirement
* leveldb
 * https://code.google.com/p/leveldb/
* jansson
 * http://www.digip.org/jansson/
* libev
 * http://software.schmorp.de/pkg/libev.html

## TODO
- [ ] Feature: Auto sharding.
 - [x] Cache out entries of current virtual bucket.
 - [ ] Implement "new shard" build sequence.
 - [ ] Implement dump pipe.
- [ ] Parse configurations.
- [ ] Implement bgwriter.
- [ ] Implement logger.
- [ ] Implement client lib.
- [ ] Implement configuration server.
