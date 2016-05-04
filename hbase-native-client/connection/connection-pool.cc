/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "connection/connection-pool.h"

#include <wangle/service/Service.h>

using std::mutex;
using std::unique_ptr;
using std::shared_ptr;
using hbase::pb::ServerName;
using folly::SharedMutexWritePriority;

namespace hbase {

ConnectionPool::ConnectionPool()
    : cf_(std::make_shared<ConnectionFactory>()), connections_(), map_mutex_() {
}
ConnectionPool::ConnectionPool(std::shared_ptr<ConnectionFactory> cf)
    : cf_(cf), connections_(), map_mutex_() {}

std::shared_ptr<HBaseService> ConnectionPool::get(const ServerName &sn) {
  // Create a read lock.
  SharedMutexWritePriority::UpgradeHolder holder(map_mutex_);

  auto found = connections_.find(sn);
  if (found == connections_.end() || found->second == nullptr) {
    // Move the upgradable lock into the write lock if the connection
    // hasn't been found.
    SharedMutexWritePriority::WriteHolder holder(std::move(holder));
    auto new_con = cf_->make_connection(sn.host_name(), sn.port());
    connections_.insert(std::make_pair(sn, new_con));
    return new_con;
  }
  return found->second;
}

void ConnectionPool::close(const ServerName &sn) {
  SharedMutexWritePriority::WriteHolder holder(map_mutex_);

  auto found = connections_.find(sn);
  if (found == connections_.end() || found->second == nullptr) {
    return;
  }
  auto service = found->second;
  connections_.erase(found);
}
}
