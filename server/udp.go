package main

import (
	"net"
	"fmt"
	"strings"
)

type EchoServer struct {
}

func NewEchoServer() *EchoServer {
	return new(EchoServer)
}

func (h *EchoServer) Start(port int) error {
	interfaces, err := net.Interfaces()
	if err != nil {
		return err
	}

	//监听所有网卡的9000端口
	for _, v := range interfaces {
		addr, _ := v.Addrs()
		if len(addr) <= 0 {
			continue
		}

		ip, _, e := net.ParseCIDR(addr[0].String())
		if e != nil || ip == nil || !strings.Contains(ip.String(), ".") {
			fmt.Printf("jump interface:%s\n", v.Name)
			continue
		}

		//原监听端口
		socket, err := net.ListenUDP("udp4", &net.UDPAddr{
			IP:   ip,
			Port: port,
		})
		if err != nil {
            fmt.Printf("ListenUDP %d error:%s\n", port, err.Error())
            return err
		}
		go h.handle(socket)
	}

	return nil
}

func (h *EchoServer) handle(socket *net.UDPConn) {
	defer socket.Close()

	for {
		data := make([]byte, 4096)
		//读取数据
		n, remoteAddr, err := socket.ReadFromUDP(data)
		if err != nil {
            fmt.Printf("read error:%s\n", err.Error())
			continue
		}

        n, err = socket.WriteToUDP(data[:n], remoteAddr)
        if err != nil {
            fmt.Printf("write error:%s\n", err.Error())
            continue
        }
        fmt.Printf("udp %s echo\n", remoteAddr)
	}
}


