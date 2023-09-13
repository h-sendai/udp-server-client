# server

server/server

port 1234 でクライアントからのUDPパケットを接続をrecvfromで待つ。
クライアントからの最初のパケット先頭にはunsigned long (ホストオーダー)
で送信するUDPパケット数がはいっているので、これをとりだして
指定された数のUDPパケットを送信する。
最初のrecvfrom()以降は
クライアント側で発生したエラーをICMPエラーで捕捉できるようにするために
connect()したUDPソケットを使っている。

```
Usage: server [-d] [-b bufsize] [-p port] [-s sleep_usec] [-z bzsleep_usec]
default bufsize: 1024 bytes.  Allow k (kilo), m (mega) suffix
default port: 1234
```

サーバーが
送るデータの最初にはシーケンス番号がint値として入っている。
読む側では、読んだ回数とデータにはいっているシーケンス番号を比較して
パケットが落ちたかどうかの判定ができる。

# client

client/client

serverに最初に送信するUDPパケットの先頭にはusinged log (ホストオーダー)
でサーバーに送ってもらうUDPパケット数をいれておく。

write()/read()が使えるようにconnect()したUDPソケットを使っている。

read()のバッファは64kB固定で確保している。UDPパケットの大きさは
サーバー（server/server）の``-b bufsize``で決まる。

# udp-read-trend

サーバーからのデータを読み、1秒おきに読んだバイト数、
Gbpsへの換算値、パケットを落とした数を表示する。
パケットがおちていても読み出しを続行する。

