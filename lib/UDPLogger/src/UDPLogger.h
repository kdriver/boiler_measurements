#ifndef mylogger
#define mylogger 

#define LOGGIT_PORT 8788

class UDPLogger {
    public:
        UDPLogger( const char *ip, unsigned short int dest_port );
        void init(void);
        void send(String s);
    private:
        char remote[100];
        unsigned short int dest_port;
};


#endif