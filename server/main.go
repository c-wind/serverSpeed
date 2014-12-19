package main

import (
	"fmt"
)

var (
)

type MainServer struct {
}

const (
	RELEASE_DATE = "2014.10.30 19:21:17"
	VERSION      = "1.0.0"
    PORT = 9003
)

func (this *MainServer) GetVersion() string {
	return fmt.Sprintf("%s %s", VERSION, RELEASE_DATE)
}



func main() {
    es := NewEchoServer()
    ts := NewTcpServer()

    err := es.Start(PORT)
    if err != nil {
        fmt.Printf("start echo error:%s\n", err.Error())
    }

    err = ts.Start(PORT)
    if err != nil {
        fmt.Printf("start tcp echo error:%s\n", err.Error())
    }
}
