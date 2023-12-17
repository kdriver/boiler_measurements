#ifndef mylogger
#define mylogger 

#define LOGGIT_PORT 8788

class UDPLogger {
    public:
        UDPLogger(IPAddress ip, unsigned short int dest_port );
        void init(String s = "noname");
        void send(String s);
    private:
        IPAddress remote;
        unsigned short int dest_port;
};


#endif