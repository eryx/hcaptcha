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
	"image"
	"image/color"
	"image/draw"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"code.hooto.com/lynkdb/iomix/connect"
	"code.hooto.com/lynkdb/iomix/skv"
	"code.hooto.com/lynkdb/kvgo"
	"github.com/eryx/freetype-go/freetype"
	"github.com/eryx/freetype-go/freetype/truetype"
	"github.com/lessos/lessgo/types"
)

const (
	ns_length_max = 30
)

type Options struct {
	//
	FontPath string `json:"font_path,omitempty"`
	DataDir  string `json:"data_dir,omitempty"`

	// Standard width and height of a CAPTCHA image.
	ImageWidth  int `json:"image_width,omitempty"`
	ImageHeight int `json:"image_height,omitempty"`

	// RGB red, green, blue values for the color of a CAPTCHA image.
	ImageColor []uint8 `json:"image_color,omitempty"`

	// Expiration time (in milliseconds) of CAPTCHAs used by store.
	ImageExpiration int64 `json:"image_expiration,omitempty"`

	// Symbols used to draw CAPTCHA
	//
	// Example:
	//   symbols 0123456789
	// Example:
	//   symbols 34578acdekpsvxy
	//   (!) alphabet without similar symbols (0=o, 1=l, 2=z, 6=b, 9=g, ...)
	Symbols string `json:"symbols,omitempty"`

	// Default number of symbols in CAPTCHA solution.
	LengthMin int `json:"length_min,omitempty"`
	LengthMax int `json:"length_max,omitempty"`

	//
	ServerPort uint16 `json:"server_port,omitempty"`

	//  symbol's vertical fluctuation amplitude
	fluctuation_amplitude float64

	//
	font_size    int
	font_height  int
	font_padding int
}

var (
	prefix        string
	err           error
	defSymbols    = "34578acdekpsvxy"
	gcfg          Options
	DefaultConfig = Options{
		ImageWidth:            160,
		ImageHeight:           80,
		ImageExpiration:       86400000,
		ImageColor:            []uint8{51, 102, 204},
		Symbols:               defSymbols,
		LengthMin:             4,
		LengthMax:             6,
		ServerPort:            9527,
		fluctuation_amplitude: 0.2,
	}
	fonts         = FontList{}
	DataConnector skv.Connector
)

func Config(cfg Options) error {

	prefix, err = filepath.Abs(filepath.Dir(os.Args[0]))
	if err != nil {
		prefix = "/opt/hcaptcha"
	}

	prefix = filepath.Clean(prefix)
	if _, err := os.Open(prefix + "/var/fonts"); err != nil {
		prefix = "./"
	}

	if DataConnector == nil {

		if cfg.DataDir == "" {
			cfg.DataDir = prefix + "/var/db"
		}

		if _, err := os.Open(cfg.DataDir); err != nil {
			return err
		}

		opts := connect.ConnOptions{
			Name:      types.NewNameIdentifier("hcaptcha"),
			Connector: "iomix/skv/Connector",
			Driver:    types.NewNameIdentifier("lynkdb/kvgo"),
		}
		opts.SetValue("data_dir", cfg.DataDir)

		if DataConnector, err = kvgo.Open(opts); err != nil {
			return err
		}
	}

	if cfg.ImageWidth < 50 {
		cfg.ImageHeight = 50
	}

	if cfg.ImageWidth > 500 {
		cfg.ImageWidth = 500
	}

	if cfg.ImageHeight < 30 {
		cfg.ImageHeight = 30
	}

	if cfg.ImageHeight > 200 {
		cfg.ImageHeight = 200
	}

	cfg.Symbols = strings.TrimSpace(cfg.Symbols)
	if cfg.Symbols == "" {
		cfg.Symbols = defSymbols
	}

	if cfg.LengthMin < 2 {
		cfg.LengthMin = 2
	}

	if cfg.LengthMax > ns_length_max {
		cfg.LengthMin = ns_length_max
	}

	if cfg.LengthMin > cfg.LengthMax {
		cfg.LengthMin = 2
	}

	for i := len(cfg.ImageColor); i < 4; i++ {
		cfg.ImageColor = append(cfg.ImageColor, 255)
	}

	if cfg.ImageExpiration < 600000 {
		cfg.ImageExpiration = 600000
	}

	cfg.fluctuation_amplitude = 0.2
	cfg.font_size = cfg.ImageHeight / 2
	cfg.font_height = (cfg.ImageHeight * 3) / 4
	cfg.font_padding = cfg.ImageHeight / 4

	gcfg = cfg

	return font_setup()
}

func font_setup() error {

	var (
		font *truetype.Font
		err  error
	)

	if gcfg.FontPath != "" {
		if data, err := ioutil.ReadFile(gcfg.FontPath); err == nil {
			font, err = freetype.ParseFont(data)
		}
	}

	if err != nil || font == nil {
		font, err = freetype.ParseFont(font_default)
		if err != nil {
			return err
		}
	}

	for _, v := range gcfg.Symbols {

		imagefont := image.NewRGBA(image.Rect(0, 0, gcfg.font_height, gcfg.font_height))
		draw.Draw(imagefont, imagefont.Bounds(), image.Transparent, image.ZP, draw.Src)

		c := freetype.NewContext()
		c.SetDst(imagefont)
		c.SetClip(imagefont.Bounds())
		c.SetSrc(image.NewUniform(color.RGBA{
			gcfg.ImageColor[0],
			gcfg.ImageColor[1],
			gcfg.ImageColor[2],
			gcfg.ImageColor[3]}))
		c.SetFont(font)
		c.SetFontSize(float64(gcfg.font_size))

		if _, err := c.DrawString(string(v), freetype.Pt(0, gcfg.font_size)); err != nil {
			continue
		}

		r := image.Rect(0, 0, 0, 0)

		for ix := 0; ix < gcfg.font_height; ix++ {

			for iy := 0; iy < gcfg.font_height; iy++ {

				if _, _, _, a := imagefont.At(ix, iy).RGBA(); a > 10 {

					if r.Min.X < 1 {
						r.Min.X, r.Min.Y = ix, iy
					} else if iy < r.Min.Y {
						r.Min.Y = iy
					}

					break
				}
			}
		}

		for ix := gcfg.font_size; ix > 0; ix-- {

			for iy := gcfg.font_height; iy > 0; iy-- {

				if _, _, _, a := imagefont.At(ix, iy).RGBA(); a > 10 {

					if r.Max.X < 1 {
						r.Max.X, r.Max.Y = ix, iy
					} else if iy > r.Max.Y {
						r.Max.Y = iy
					}

					break
				}
			}
		}

		imagefontdump := image.NewRGBA(image.Rect(0, 0, r.Max.X-r.Min.X, r.Max.Y-r.Min.Y))

		draw.Draw(imagefontdump, imagefontdump.Bounds(), imagefont, r.Min, draw.Over)

		fonts.Items = append(fonts.Items, &FontEntry{
			Symbol: string(v),
			Width:  r.Max.X - r.Min.X,
			Height: r.Max.Y - r.Min.Y,
			Image:  imagefontdump,
		})

		fonts.Length++

		if r.Max.Y-r.Min.Y > fonts.MaxHeight {
			fonts.MaxHeight = r.Max.Y - r.Min.Y
		}
	}

	return nil
}
