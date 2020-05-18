#ifndef StringHandler_h
#define StringHandler_h

enum Grammar { CommandPrimitive, VariablePrimitive , Equals , Value , Done };
enum Command { Get,Set };
enum Variable { off_threshold,on_threshold };

class StringHandler {
    public:
        StringHandler(const char *s);
        ~StringHandler();
        unsigned  int tokenise(void);
        const char *get_token(unsigned int);
        bool validate(void);
    private:
        char string_copy[501];
        char tokens[5][100];
        enum { max_input_length=500 };
    unsigned int num_tokens;
    
    enum Command command;
    enum Variable variable;
    unsigned int  value;
};

#endif
