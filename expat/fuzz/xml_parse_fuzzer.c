/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdint.h>

#ifdef __APPLE__
#include <limits.h>
#include <string.h>
#endif

#include "expat.h"
#include "siphash.h"

// Macros to convert preprocessor macros to string literals. See
// https://gcc.gnu.org/onlinedocs/gcc-3.4.3/cpp/Stringification.html
#define xstr(s) str(s)
#define str(s) #s

#ifndef __APPLE__
// The encoder type that we wish to fuzz should come from the compile-time
// definition `ENCODING_FOR_FUZZING`. This allows us to have a separate fuzzer
// binary for
#ifndef ENCODING_FOR_FUZZING
#  error "ENCODING_FOR_FUZZING was not provided to this fuzz target."
#endif
#endif /* __APPLE__ */

// 16-byte deterministic hash key.
static unsigned char hash_key[16] = "FUZZING IS FUN!";

static void XMLCALL
start(void *userData, const XML_Char *name, const XML_Char **atts) {
  (void)userData;
#ifdef __APPLE__
  (void)strlen(name);
  for (unsigned i = 0; atts[i] != NULL; i++)
      (void)strlen(atts[i]);
#else
  (void)name;
  (void)atts;
#endif
}
static void XMLCALL
end(void *userData, const XML_Char *name) {
  (void)userData;
#ifdef __APPLE__
  (void)strlen(name);
#else
  (void)name;
#endif
}

#ifdef __APPLE__
static const char *encodings[] = {
  "US-ASCII",
  "ISO-8859-1",
  "UTF-8",
  "UTF-16",
  "UTF-16BE",
  "UTF-16LE",
  "UNKNOWN_ENCODING",
};
const unsigned encodingsCount = sizeof(encodings) / sizeof(const char*);
#endif /* __APPLE__ */

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
#ifdef __APPLE__
  XML_Parser p = XML_ParserCreate(encodings[size % encodingsCount]);
#else
  XML_Parser p = XML_ParserCreate(xstr(ENCODING_FOR_FUZZING));
#endif
  assert(p);

  // Set the hash salt using siphash to generate a deterministic hash.
  struct sipkey *key = sip_keyof(hash_key);
  XML_SetHashSalt(p, (unsigned long)siphash24(data, size, key));

  XML_SetElementHandler(p, start, end);
#ifdef __APPLE__
  assert(size <= INT_MAX);
  XML_Parse(p, (const XML_Char *)data, (int)size, 0);
  XML_Parse(p, (const XML_Char *)data, (int)size, 1);
#else
  XML_Parse(p, (const XML_Char *)data, size, 0);
  XML_Parse(p, (const XML_Char *)data, size, 1);
#endif
  XML_ParserFree(p);
  return 0;
}
