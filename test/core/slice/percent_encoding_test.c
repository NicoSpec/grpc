/*
 *
 * Copyright 2016, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "src/core/lib/slice/percent_encoding.h"

#include <grpc/support/alloc.h>
#include <grpc/support/log.h>

#include "src/core/lib/slice/slice_string_helpers.h"
#include "src/core/lib/support/string.h"
#include "test/core/util/test_config.h"

#define TEST_VECTOR(raw, encoded, dict) \
  test_vector(raw, sizeof(raw) - 1, encoded, sizeof(encoded) - 1, dict)

#define TEST_NONCONFORMANT_VECTOR(encoded, permissive_unencoded, dict) \
  test_nonconformant_vector(encoded, sizeof(encoded) - 1,              \
                            permissive_unencoded,                      \
                            sizeof(permissive_unencoded) - 1, dict)

static void test_vector(const char *raw, size_t raw_length, const char *encoded,
                        size_t encoded_length, const uint8_t *dict) {
  char *raw_msg = gpr_dump(raw, raw_length, GPR_DUMP_HEX | GPR_DUMP_ASCII);
  char *encoded_msg =
      gpr_dump(encoded, encoded_length, GPR_DUMP_HEX | GPR_DUMP_ASCII);
  gpr_log(GPR_DEBUG, "Trial:\nraw = %s\nencoded = %s", raw_msg, encoded_msg);
  gpr_free(raw_msg);
  gpr_free(encoded_msg);

  grpc_slice raw_slice = grpc_slice_from_copied_buffer(raw, raw_length);
  grpc_slice encoded_slice =
      grpc_slice_from_copied_buffer(encoded, encoded_length);
  grpc_slice raw2encoded_slice = grpc_percent_encode_slice(raw_slice, dict);
  grpc_slice encoded2raw_slice;
  GPR_ASSERT(grpc_strict_percent_decode_slice(encoded_slice, dict,
                                              &encoded2raw_slice));
  grpc_slice encoded2raw_permissive_slice =
      grpc_permissive_percent_decode_slice(encoded_slice);

  char *raw2encoded_msg =
      grpc_dump_slice(raw2encoded_slice, GPR_DUMP_HEX | GPR_DUMP_ASCII);
  char *encoded2raw_msg =
      grpc_dump_slice(encoded2raw_slice, GPR_DUMP_HEX | GPR_DUMP_ASCII);
  char *encoded2raw_permissive_msg = grpc_dump_slice(
      encoded2raw_permissive_slice, GPR_DUMP_HEX | GPR_DUMP_ASCII);
  gpr_log(GPR_DEBUG,
          "Result:\nraw2encoded = %s\nencoded2raw = %s\nencoded2raw_permissive "
          "= %s",
          raw2encoded_msg, encoded2raw_msg, encoded2raw_permissive_msg);
  gpr_free(raw2encoded_msg);
  gpr_free(encoded2raw_msg);
  gpr_free(encoded2raw_permissive_msg);

  GPR_ASSERT(0 == grpc_slice_cmp(raw_slice, encoded2raw_slice));
  GPR_ASSERT(0 == grpc_slice_cmp(raw_slice, encoded2raw_permissive_slice));
  GPR_ASSERT(0 == grpc_slice_cmp(encoded_slice, raw2encoded_slice));

  grpc_slice_unref(encoded2raw_slice);
  grpc_slice_unref(encoded2raw_permissive_slice);
  grpc_slice_unref(raw2encoded_slice);
  grpc_slice_unref(raw_slice);
  grpc_slice_unref(encoded_slice);
}

static void test_nonconformant_vector(const char *encoded,
                                      size_t encoded_length,
                                      const char *permissive_unencoded,
                                      size_t permissive_unencoded_length,
                                      const uint8_t *dict) {
  char *permissive_unencoded_msg =
      gpr_dump(permissive_unencoded, permissive_unencoded_length,
               GPR_DUMP_HEX | GPR_DUMP_ASCII);
  char *encoded_msg =
      gpr_dump(encoded, encoded_length, GPR_DUMP_HEX | GPR_DUMP_ASCII);
  gpr_log(GPR_DEBUG, "Trial:\nraw = %s\nencoded = %s", permissive_unencoded_msg,
          encoded_msg);
  gpr_free(permissive_unencoded_msg);
  gpr_free(encoded_msg);

  grpc_slice permissive_unencoded_slice = grpc_slice_from_copied_buffer(
      permissive_unencoded, permissive_unencoded_length);
  grpc_slice encoded_slice =
      grpc_slice_from_copied_buffer(encoded, encoded_length);
  grpc_slice encoded2raw_slice;
  GPR_ASSERT(!grpc_strict_percent_decode_slice(encoded_slice, dict,
                                               &encoded2raw_slice));
  grpc_slice encoded2raw_permissive_slice =
      grpc_permissive_percent_decode_slice(encoded_slice);

  char *encoded2raw_permissive_msg = grpc_dump_slice(
      encoded2raw_permissive_slice, GPR_DUMP_HEX | GPR_DUMP_ASCII);
  gpr_log(GPR_DEBUG, "Result:\nencoded2raw_permissive = %s",
          encoded2raw_permissive_msg);
  gpr_free(encoded2raw_permissive_msg);

  GPR_ASSERT(0 == grpc_slice_cmp(permissive_unencoded_slice,
                                 encoded2raw_permissive_slice));

  grpc_slice_unref(permissive_unencoded_slice);
  grpc_slice_unref(encoded2raw_permissive_slice);
  grpc_slice_unref(encoded_slice);
}

int main(int argc, char **argv) {
  grpc_test_init(argc, argv);
  TEST_VECTOR(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~",
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~",
      grpc_url_percent_encoding_unreserved_bytes);
  TEST_VECTOR("\x00", "%00", grpc_url_percent_encoding_unreserved_bytes);
  TEST_VECTOR("\x01", "%01", grpc_url_percent_encoding_unreserved_bytes);
  TEST_VECTOR("a b", "a%20b", grpc_url_percent_encoding_unreserved_bytes);
  TEST_VECTOR(" b", "%20b", grpc_url_percent_encoding_unreserved_bytes);
  TEST_VECTOR("a b", "a b", grpc_compatible_percent_encoding_unreserved_bytes);
  TEST_VECTOR(" b", " b", grpc_compatible_percent_encoding_unreserved_bytes);
  TEST_VECTOR("\x0f", "%0F", grpc_url_percent_encoding_unreserved_bytes);
  TEST_VECTOR("\xff", "%FF", grpc_url_percent_encoding_unreserved_bytes);
  TEST_VECTOR("\xee", "%EE", grpc_url_percent_encoding_unreserved_bytes);
  TEST_NONCONFORMANT_VECTOR("%", "%",
                            grpc_url_percent_encoding_unreserved_bytes);
  TEST_NONCONFORMANT_VECTOR("%A", "%A",
                            grpc_url_percent_encoding_unreserved_bytes);
  TEST_NONCONFORMANT_VECTOR("%AG", "%AG",
                            grpc_url_percent_encoding_unreserved_bytes);
  TEST_NONCONFORMANT_VECTOR("\0", "\0",
                            grpc_url_percent_encoding_unreserved_bytes);
  return 0;
}