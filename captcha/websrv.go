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
	"github.com/lessos/lessgo/httpsrv"
)

func WebServerModule() httpsrv.Module {

	module := httpsrv.NewModule("main")

	module.ControllerRegister(new(Api))

	return module
}

func WebServerStart() {

	httpsrv.GlobalService.Config.HttpPort = gcfg.ServerPort

	httpsrv.GlobalService.ModuleRegister("/hcaptcha", WebServerModule())

	httpsrv.GlobalService.Start()
}

type Api struct {
	*httpsrv.Controller
}

func (c Api) VerifyAction() {

	if err := Verify(c.Params.Get("hcaptcha_token"),
		c.Params.Get("hcaptcha_word")); err != nil {
		c.RenderString("false\n" + err.Error())
	} else {
		c.RenderString("true")
	}
}

func (c Api) ImageAction() {

	c.AutoRender = false

	reload := false

	if c.Params.Get("hcaptcha_opt") == "refresh" {
		reload = true
	}

	if img, err := ImageFetch(c.Params.Get("hcaptcha_token"), reload); err != nil {
		c.RenderError(500, err.Error())
	} else {
		c.Response.Out.Header().Set("Content-type", "image/png")
		c.Response.Out.Write(img)
	}
}
