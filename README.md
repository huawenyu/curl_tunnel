# curl_tunnel
A proxy tunnel using libcurl, fork https://repo.or.cz/w/curltunnel.git

# Usage

	echo hello | curltunnel -p "<proxy>:<port>" -u "a:a" -d "www.google.com:80"

# Build

## ubuntu

Choose one mode to install libcurl, depending on whether you want to use for SSL:
- gnutls,
- openssl,
- nss,

	$ sudo apt-get install libcurl4-gnutls-dev
	$ sudo apt-get install libcurl4-openssl-dev
	$ sudo apt-get install libcurl4-nss-dev
	$ make

