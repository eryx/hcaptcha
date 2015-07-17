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
	"bytes"
	// "fmt"
	"image"
	"image/draw"
	"image/png"
	"math"
	"math/rand"

	"github.com/lessos/lessgo/httpsrv"
)

func WebServerStart() {

	httpsrv.GlobalService.Config.HttpPort = gcfg.ServerPort

	module := httpsrv.NewModule("main")

	module.ControllerRegister(new(Api))

	httpsrv.GlobalService.ModuleRegister("/hcaptcha", module)

	httpsrv.GlobalService.Start()
}

type Api struct {
	*httpsrv.Controller
}

func (c Api) VerifyAction() {

	c.AutoRender = false

	var (
		token = c.Params.Get("hcaptcha_token")
		word  = c.Params.Get("hcaptcha_word")
	)

	if token == "" || word == "" {
		c.RenderString("false\ninvalid-request")
		return
	}

	if store == nil {
		c.RenderString("false\nhcaptcha-not-reachable")
		return
	}

	//
	if rs := store.Get(_token_word_key(token)); rs.Status == "OK" && rs.String() == word {
		c.RenderString("true")
	} else {
		c.RenderString("false\nincorrect-hcaptcha-word")
	}

	store.Del(_token_word_key(token), _token_image_key(token))
}

func (c Api) ImageAction() {

	c.AutoRender = false

	var (
		token  = c.Params.Get("hcaptcha_token")
		option = c.Params.Get("hcaptcha_opt")
	)

	if option != "refresh" {
		if rs := store.Get(_token_image_key(token)); rs.Status == "OK" {
			c.Response.Out.Header().Set("Content-type", "image/png")
			c.Response.Out.Write(rs.Bytes())
			return
		}
	}

	vylen := gcfg.LengthMin + rand.Intn(gcfg.LengthMax-gcfg.LengthMin+1)

	capstr := image.NewRGBA(image.Rect(0, 0, gcfg.ImageWidth, gcfg.ImageHeight))

	prev_min_x, prev_min_y, prev_max_x, prev_max_y := 0, 0, 0, 0

	vyword := ""

	for i := 0; i < vylen; i++ {

		font := fonts.Items[rand.Intn(fonts.Length)]

		yshift := rand.Intn(int(float64(font.Height) * (1 - (2 * gcfg.fluctuation_amplitude))))

		start := gcfg.font_size - int(float64(font.Height)*(1-gcfg.fluctuation_amplitude))

		var r image.Rectangle

		if i == 0 {

			prev_min_x, prev_min_y = gcfg.font_padding, start+yshift
			prev_max_x, prev_max_y = prev_min_x+font.Width, prev_min_y+font.Height

			r = image.Rect(prev_min_x, prev_min_y, prev_max_x, prev_max_y)

		} else {

			x, y := prev_max_x, start+yshift

			for sx := 1; sx < font.Width; sx += 1 {

				for sy := 1; sy < font.Height; sy += 1 {

					if _, _, _, a := font.Image.At(sx, sy).RGBA(); a < 5 {
						continue
					}

					target_x, target_y := prev_max_x-sx, start+yshift+sy

					if _, _, _, al := capstr.At(target_x, target_y).RGBA(); al < 5 {
						continue
					}

					x = target_x

					break
				}

				if x != prev_max_x {
					break
				}
			}

			prev_max_x = x + font.Width

			r = image.Rect(x, y, prev_max_x, y+font.Height)
		}

		if prev_max_x > (gcfg.ImageWidth - 10) {
			break
		}

		vyword += font.Symbol
		draw.Draw(capstr, r, font.Image, image.Pt(0, 0), draw.Over)
	}

	capwave := image.NewRGBA(image.Rect(0, 0, gcfg.ImageWidth, gcfg.ImageHeight))

	amplude := _rand_float(5, 10)
	period := _rand_float(100, 200)

	dx := 2.5 * math.Pi / period

	for x := 0; x < gcfg.ImageWidth; x++ {

		for y := 0; y < gcfg.ImageHeight; y++ {

			sx := x + int(amplude*math.Sin(float64(y)*dx))
			sy := y + int(amplude*math.Cos(float64(x)*dx))

			if sx < 0 || sy < 0 || sx >= gcfg.ImageWidth-1 || sy >= gcfg.ImageHeight-1 {
				continue
			}

			if capstr.RGBAAt(sx, sy).A < 1 {
				continue
			}

			capwave.Set(x, y, capstr.At(sx, sy))
		}
	}

	buf := new(bytes.Buffer)

	if err := png.Encode(buf, capwave); err != nil {
		return
	}

	if rs := store.Setex(_token_word_key(token), []byte(vyword), gcfg.imageExpirationMS); rs.Status != "OK" {
		// fmt.Println("DB Error", rs.Status)
		return
	}

	if rs := store.Setex(_token_image_key(token), buf.Bytes(), gcfg.imageExpirationMS); rs.Status != "OK" {
		// fmt.Println("DB Error", rs.Status)
		return
	}

	c.Response.Out.Write(buf.Bytes())
}
