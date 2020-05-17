#ifndef StringHandler_h
#define StringHandler_h


class StringHandler {
    public:
        StringHandler(const char *s);
        ~StringHandler();
        unsigned  int tokenise(void);
        const char *get_token(unsigned int);
    private:
        char string_copy[501];
        char tokens[5][100];
        enum { max_input_length=500 };
};

#endif
