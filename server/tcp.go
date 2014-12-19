package main

import (
    "fmt"
    "net"
)

type TcpServer struct {
    ln        net.Listener
}


func NewTcpServer() *TcpServer {
    this := new(TcpServer)
    return this
}

func (this *TcpServer) Start(port int) error {
    addr := fmt.Sprintf(":%d", port)
    var err error
    this.ln, err = net.Listen("tcp4", addr)
    if err != nil {
        return err
    }

    this.startAccept()
    return nil
}

func (this *TcpServer) startAccept() {
    defer this.ln.Close()

    for {
        conn, err := this.ln.Accept()
        if err != nil {
            fmt.Printf("accept error:%s\n", err.Error())
            continue
        }

        left, ok := conn.(*net.TCPConn)
        if ok {
            go this.handleConnection(left)
        } else {
            fmt.Printf("conn to socket error\n")
        }
    }

    fmt.Printf("accept disable\n")
}

func (this *TcpServer) handleConnection(left *net.TCPConn) {
    data := make([]byte, 4)
    for {
        n, err := left.Read(data)
        if err != nil {
            fmt.Printf("read %s error:%s\n", left.RemoteAddr().String(), err.Error())
            return
        }

        n, err = left.Write(data[:n])
        if err != nil {
            fmt.Printf("write %s error:%s\n", left.RemoteAddr().String(), err.Error())
            return
        }

        fmt.Printf("tcp %s ok\n", left.RemoteAddr().String())
    }
}

