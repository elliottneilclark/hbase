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
#include "core/header-handler.h"

#include "if/HBase.pb.h"
#include "if/RPC.pb.h"

using namespace hbase;

const static std::string PREAMBLE = "HBas";

folly::Future<folly::Unit>
HeaderHandler::write(Context *ctx, std::unique_ptr<folly::IOBuf> msg) {
  printf("CTX -> %p", ctx);
  if (need_send_header_) {
    need_send_header_ = false;
    write_header(ctx);
  }
  return ctx->fireWrite(std::move(msg));
}

folly::Future<folly::Unit> HeaderHandler::write_header(Context *ctx) {
  pb::ConnectionHeader h;
  h.mutable_user_info()->set_effective_user("elliott");
  h.set_service_name("ClientService");

  auto magic = folly::IOBuf::copyBuffer(PREAMBLE);
  auto buf = folly::IOBuf::create(6);
  auto msg = folly::IOBuf::copyBuffer(h.SerializeAsString());

  buf->append(6);
  folly::io::RWPrivateCursor c(buf.get());
  c.write((uint8_t)0);
  c.write((uint8_t)80);
  c.write((uint32_t)h.ByteSize());

  buf->prependChain(std::move(msg));
  magic->prependChain(std::move(buf));

  return ctx->fireWrite(std::move(magic));
}
