# curl_tunnel
A proxy tunnel using libcurl, fork https://repo.or.cz/w/curltunnel.git

# Usage

	echo hello | curltunnel -p "<proxy>:<port>" -u "a:a" -d "www.google.com:80"

# Build

## ubuntu

Choose one mode to install libcurl, depending on which we want to use for SSL:
+ gnutls,
+ openssl,
+ nss,

```bash
	$ sudo apt-get install libcurl4-gnutls-dev
	$ sudo apt-get install libcurl4-openssl-dev
	$ sudo apt-get install libcurl4-nss-dev
	$ make
```

# Design

Using iptable redirect the outbout traffic to local-in,
  then `curltunnel` listen on this local-in port as the dummy-server side,
  bypass the following traffic after setup tunnel to proxy/network-device by HTTP/HTTPS.

Firstly, let check iptables works:

    (LinuxPC) $ sudo iptables -t nat -A OUTPUT -p tcp --dport 54321 -j DNAT --to-destination 127.0.0.1:4444

    (LinuxPC-shell_1) $ netcat -l 4444                  ### setup local-in listen
    (LinuxPC-shell_2) $ netcat www.google.com 54321     ### try to connect outboud public server
    ... try to input something <enter> ...

    (LinuxPC-shell_1) $
    ... Will show up the string input by shell_2 ...


```C:  Get the orginal server's IP address
    // Get the original server address
    struct sockaddr_in addr;
    socklen_t addr_sz = sizeof(addr);
    memset(&addr, 0, addr_sz);
    addr.sin_family = AF_INET;
    getsockopt(clientfd, SOL_IP, 80/*SO_ORIGINAL_DST*/, &addr, &addr_sz);
    printf("The original server address is %s\n", inet_ntoa(addr.sin_addr));
```
