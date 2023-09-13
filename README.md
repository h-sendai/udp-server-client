# server

server/server

port 1234 でクライアントからのUDPパケットを接続をrecvfromで待つ。
クライアントからの最初のパケット先頭には

```
struct arg_to_server {
    int bufsize;
    int sleep_usec;
    int bzsleep_usec;
} 
```

の構造体がはいっているのでこれをデコードして、1回に送る
UDPパケットサイズを決定する。

最初のrecvfrom()以降は
クライアント側で発生したエラーをICMPエラーで捕捉できるようにするために
connect()したUDPソケットを使っている。

クライアント側のソケットがclose()されると
```
2023-09-13 15:51:11.013184 Connection refused
```
とエラーメッセージを出す。これはconnect()edソケットを使ってwrite()
しているのでクライアント側でソケットがclose()されたら
ICMP 192.168.0.1 udp port 57616 unreachable
のようにICMPでソケットがクローズされたことがわかるからである。


```
Usage: server [-d] [-b bufsize] [-p port] [-s sleep_usec] [-z bzsleep_usec]
default bufsize: 1024 bytes.  Allow k (kilo), m (mega) suffix
default port: 1234
```

サーバーが
送るデータの最初にはシーケンス番号がint値として入っている。
読む側では、読んだ回数とデータにはいっているシーケンス番号を比較して
パケットが落ちたかどうかの判定ができる。

# udp-read-trend

サーバーからのデータを読み、1秒おきに読んだバイト数、
Gbpsへの換算値、パケットを落とした数を表示する。
パケットがおちていても読み出しを続行する。

最初のUDPパケットとして次の構造体をサーバーに送る。

```
struct arg_to_server {
    int bufsize;
    int sleep_usec;
    int bzsleep_usec;
} 
```

write()/read()が使えるようにconnect()したUDPソケットを使っている。
read()のバッファは64kB固定で確保している。UDPパケットの大きさは
サーバー（server/server）の``-b bufsize``で決まる。

# client

client/client

これは上のサーバー、udp-read-trendを書く前のクライアントで
上の構造体には対応していない(のでたぶん動作しない)。

serverに最初に送信するUDPパケットの先頭にはusinged log (ホストオーダー)
でサーバーに送ってもらうUDPパケット数をいれておく。

write()/read()が使えるようにconnect()したUDPソケットを使っている。

read()のバッファは64kB固定で確保している。UDPパケットの大きさは
サーバー（server/server）の``-b bufsize``で決まる。



