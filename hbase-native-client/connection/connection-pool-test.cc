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

#include <folly/Logging.h>
#include <gmock/gmock.h>

#include "connection/connection-factory.h"
#include "if/HBase.pb.h"

using namespace hbase;

using hbase::pb::ServerName;
using ::testing::Return;
using ::testing::_;

class MockConnectionFactory : public ConnectionFactory {
public:
  MOCK_METHOD2(make_connection,
               std::shared_ptr<HBaseService>(const std::string &hostname,
                                             int port));
};

class MockServiceBase : public HBaseService {
public:
  folly::Future<Response> operator()(std::unique_ptr<Request> req) override {
    return do_operation(req.get());
  }
  virtual folly::Future<Response> do_operation(Request *req) {
    return folly::makeFuture<Response>(Response{});
  }
};

class MockService : public MockServiceBase {
public:
  MOCK_METHOD1(do_operation, folly::Future<Response>(Request *));
};

TEST(TestConnectionPool, TestOnlyCreateOnce) {
  std::string hostname{"hostname"};
  auto mock_service = std::make_shared<MockService>();
  uint32_t port{999};

  LOG(ERROR) << "About to make a MockConnectionFactory";
  auto mock_cf = std::make_shared<MockConnectionFactory>();
  EXPECT_CALL((*mock_cf), make_connection(_, _))
      .Times(1)
      .WillRepeatedly(Return(mock_service));
  ConnectionPool cp{mock_cf};

  LOG(ERROR) << "Created ConnectionPool";

  ServerName sn;
  sn.set_host_name(hostname);
  sn.set_port(port);

  auto result = cp.get(sn);
  ASSERT_TRUE(result != nullptr);
  result = cp.get(sn);
}
TEST(TestConnectionPool, TestOnlyCreateMultipleDispose) {
  std::string hostname_one{"hostname"};
  std::string hostname_two{"hostname_two"};
  uint32_t port{999};

  auto mock_cf = std::make_shared<MockConnectionFactory>();
  EXPECT_CALL((*mock_cf), make_connection(_, _))
      .Times(2)
      .WillRepeatedly(Return(std::make_shared<MockService>()));

  LOG(ERROR) << "Created ConnectionPool";

  ServerName sn_one;
  sn_one.set_host_name(hostname_one);
  sn_one.set_port(port);

  ServerName sn_two;
  sn_two.set_host_name(hostname_two);
  sn_two.set_port(port);

  {
    ConnectionPool cp{mock_cf};
    auto result_one = cp.get(sn_one);
    auto result_two = cp.get(sn_two);
  }
}
