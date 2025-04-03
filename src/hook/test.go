package main

import (
	"flag"
	"log"
	"os"
)

func main() {
	pos := flag.Int64("pos", 0, "position")
	data := flag.String("data", "", "data")

	flag.Parse()

	fd, err := os.OpenFile("/tmp/test.txt", os.O_RDWR|os.O_CREATE|os.O_SYNC, os.ModePerm)
	if err != nil {
		log.Fatal(err)
	}
	defer fd.Close()

	if (*pos) > 0 {
		_, err := fd.Seek(*pos, 0)
		if err != nil {
			log.Fatal(err)
		}
	}

	_, err = fd.Write([]byte(*data))
	if err != nil {
		log.Fatal(err)
	}

	log.Println("OK")
}
