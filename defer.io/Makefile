TARGET = deferio

OBJS = Config.o Data/Cache.o Data/DB.o Data/Document.o Data/JSON.o Data/JSON.Modifiers.o Data/JSON.Path.o Data/Key.o Data/xxhash.o main.o Transport/Client.o Transport/Server.o Worker/Job.o Worker/Status.o Worker/System.o Worker/Thread.o Worker/ThreadPool.o 

CFLAGS = -std=gnu++11 -g -Wall -ITransport -IWorker -IData -I/opt/local/include
CPPFLAGS = -std=gnu++11 -g -Wall -ITransport -IWorker -IData -I/opt/local/include


all: $(OBJS)
	g++ -g -Wall -o $(TARGET) $(OBJS) -lleveldb -lev -lpthread

clean:
	rm -f $(OBJS) $(TARGET)


