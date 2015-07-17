// Copyright 2015 hooto.com, All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package captcha

import (
	"crypto/md5"
	"io"
	"math/rand"
	"runtime"
	"time"
)

func init() {
	rand.Seed(time.Now().UTC().UnixNano())
	runtime.GOMAXPROCS(runtime.NumCPU())
}

func _rand_float(from, to float64) float64 {
	return (to-from)*rand.Float64() + from
}

func _token_word_key(token string) []byte {
	return append(_token_key_filter(token), 0x01)
}

func _token_image_key(token string) []byte {
	return append(_token_key_filter(token), 0x02)
}

func _token_key_filter(token string) []byte {

	if len(token) > 36 {
		token = token[:36]
	}

	h := md5.New()
	io.WriteString(h, token)

	return h.Sum(nil)
}
