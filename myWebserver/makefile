server: main.cpp ./threadpool/pool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/lock.h ./user/redis.h ./user/redis.cpp
	g++ -o server main.cpp ./threadpool/pool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/lock.h ./user/redis.h ./user/redis.cpp -lpthread -lhiredis -std=c++11

clean:
	rm  -r server
