package main

import (
	"fmt"
	"math"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/wkqco33/tcamviewer/pkg/tcamviewer"
)

func main() {
	cols, rows, err := tcamviewer.GetTerminalSize()
	if err != nil {
		cols, rows = 80, 24
	}

	renderer, err := tcamviewer.NewRenderer(tcamviewer.Config{
		TargetCols: cols,
		TargetRows: rows,
		UseDiff:    true,
		AltScreen:  true,
		HideCursor: true,
	})
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create renderer: %v\n", err)
		os.Exit(1)
	}
	defer renderer.Close()

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)

	pixelW := cols
	pixelH := rows * 2
	rgb := make([]byte, pixelW*pixelH*3)

	ticker := time.NewTicker(33 * time.Millisecond) // ~30 FPS
	defer ticker.Stop()

	frame := 0
	fmt.Printf("Go Terminal Video Player Demo running (%dx%d cells). Press Ctrl+C to exit.\n", cols, rows)

	for {
		select {
		case <-sigCh:
			return
		case <-ticker.C:
			t := float64(frame) * 0.08
			for y := 0; y < pixelH; y++ {
				for x := 0; x < pixelW; x++ {
					nx := float64(x) / float64(pixelW)
					ny := float64(y) / float64(pixelH)

					r := byte((math.Sin(nx*math.Pi*2.0+t)*0.5 + 0.5) * 255.0)
					g := byte((math.Sin(ny*math.Pi*2.0+t*1.3)*0.5 + 0.5) * 255.0)
					b := byte((math.Cos((nx+ny)*math.Pi+t*0.7)*0.5 + 0.5) * 255.0)

					idx := (y*pixelW + x) * 3
					rgb[idx+0] = r
					rgb[idx+1] = g
					rgb[idx+2] = b
				}
			}

			_ = renderer.RenderRGB(rgb, pixelW, pixelH, pixelW*3)
			frame++
			if frame >= 60 { // run 2 seconds in non-interactive mode
				return
			}
		}
	}
}
