package main

import (
	"fmt"
	"log"
	"os"
)

func main() {
	fd, err := os.OpenFile("/dev/ramdev", os.O_RDWR|os.O_CREATE|os.O_SYNC, os.ModePerm)
	if err != nil {
		log.Fatal(err)
	}
	defer fd.Close()

	data1 := []byte("hello")
	data2 := []byte("world")
	if _, err = fd.WriteAt(data1, 0); err != nil {
		log.Fatal(err)
	}
	if _, err = fd.WriteAt(data2, 4096); err != nil {
		log.Fatal(err)
	}

	out := make([]byte, 512)
	n, err := fd.ReadAt(out, 0)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("read at 0: %s\n", string(out[:n]))

	out = make([]byte, 512)
	n, err = fd.ReadAt(out, 4096)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("read at 1024: %s\n", string(out[:n]))
}
