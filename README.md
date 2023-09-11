# server

server/server

port 1234 でclientからの接続をrecvfromで待つ。
クライアントから接続があったあとはどんどんudp packetを投げる。
```
Usage: server [-d] [-b bufsize] [-c max_write_ocounter] [-p port] [-s sleep_usec] [-z bzsleep_usec]
default bufsize: 1024 bytes.  Allow k (kilo), m (mega) suffix
default port: 1234
```

送るデータの最初にはシーケンス番号がint値として入っている。
読む側では、読んだ回数とデータにはいっているシーケンス番号を比較して
パケットが落ちたかどうかの判定ができる。

# client

client/client

serverに最初1回sendtoでメッセージを送る。
あとはどんどんrcvffromで読む。

read()のバッファは64kB固定で確保している。UDPパケットの大きさは
サーバー（server/server）の``-b bufsize``で決まる。

# udp-read-trend

サーバーからのデータを読み、1秒おきに読んだバイト数、
Gbpsへの換算値、パケットを落とした数を表示する。
パケットがおちていても読み出しを続行する。

