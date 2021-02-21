#ifndef StringHandler_h
#define StringHandler_h

#define COMMAND_PORT 8788

enum Grammar { CommandPrimitive, VariablePrimitive , Equals , Value , Done };

struct CommandSet { 
  const char *cmd;
  unsigned int name; 
  unsigned int tokens;
};
struct AttributeSet {
  const char *attr;
  unsigned int name;
};
class StringHandler {
    public:
        StringHandler(const char *s, unsigned int num_cmds, CommandSet *commands,unsigned int num_attr, AttributeSet attributes[]);
        ~StringHandler();
        unsigned  int tokenise(void);
        const char *get_token(unsigned int);
        bool validate(void);
        unsigned int get_command(void);
        unsigned int get_attribute(void);
        unsigned int get_value(void);
        float get_f_value(void);
    private:
        char string_copy[501];
        char tokens[5][100];
        enum { max_input_length=500 };
        CommandSet *commands;
        unsigned int the_command;
        unsigned int num_commands;
        AttributeSet *attributes;
        unsigned int the_attribute;
        unsigned int num_attributes;
        unsigned int num_tokens;
        unsigned int the_value;
        float the_f_value;
    
    unsigned int  value;
};

#endif
