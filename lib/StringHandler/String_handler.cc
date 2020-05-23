#include "StringHandler.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


StringHandler::StringHandler(const char *text,unsigned int num_cmds, CommandSet *cmds,unsigned int num_attr, AttributeSet *attrs)
{
    unsigned int len;
    // copy the string into an internal buffer
    if ( strlen(text) < max_input_length )
    {
        strncpy(string_copy,text,500);
    }
    len = strlen(string_copy);
    for (unsigned int i=0;i<len;i++)
        string_copy[i] = toupper(string_copy[i]);
    
    num_tokens =0;
    num_commands = num_cmds;
    commands = cmds;
    num_attributes = num_attr;
    attributes = attrs;
}

StringHandler::~StringHandler()
{

}

 
unsigned int StringHandler::tokenise(void)
{
    unsigned int token=0;
    char *input_iterator = string_copy;
    char *dest_iterator;
    enum TokenState  { Searching,Collecting};
    TokenState now;
    now = Searching;

    dest_iterator = &tokens[token][0];
    // skip over any leading white space
    do {
            switch(*input_iterator)
            {
                case '\n':
                case ' ':
                    if ( now == Collecting )
                    {
                        *dest_iterator = 0;
                        token = token + 1;
                        dest_iterator = &tokens[token][0];
                        now = Searching;
                    }
                    input_iterator = input_iterator + 1;
                    break;
                case 0:
                    break;
                case '=':
                    if ( now == Collecting )
                    {
                        *dest_iterator = 0;
                        token = token + 1;
                        dest_iterator = &tokens[token][0];
                    }
                        
                    dest_iterator[0] = '=';
                    dest_iterator[1] = 0;
                    token = token + 1;
                    dest_iterator = &tokens[token][0];
                    input_iterator = input_iterator + 1;
                    break;
                default:
                    now = Collecting;
                    *dest_iterator = *input_iterator;
                    dest_iterator = dest_iterator + 1 ;
                    input_iterator = input_iterator + 1;
                    break;
            }
          
    }  while ((*input_iterator != 0 ) && (token < 5 ));
    
    num_tokens = token;
    
    return token;
}
const char *StringHandler::get_token(unsigned int token)
{
    // forst token is at 0 index
    return &tokens[token][0];
}

bool StringHandler::validate(void)
{
    unsigned int token=0;
    char *t;
    bool matched;
    
    Grammar state;
    state = CommandPrimitive;
    unsigned int i;
    int expected_tokens;
    
    
    do {
        matched = false;
        t = tokens[token];
        switch (state)
        {
            case CommandPrimitive:
                for (  i = 0 ; i < num_commands && matched == false ; i++ )
                {
                        if ( !strcmp(t,commands[i].cmd))
                        {
                            the_command = commands[i].name;
                            matched = true;
                            expected_tokens =commands[i].tokens;
                        }
                }
                if (matched == true )
                {
                    if ( expected_tokens > 1 )
                        state = VariablePrimitive;
                    else
                        state = Done;
                }
                else
                    state = Done;
            break;
            case VariablePrimitive:
                for (  i = 0 ; i < num_attributes && matched == false ; i++ )
                {
                        if ( !strcmp(t,attributes[i].attr))
                        {
                            the_attribute = attributes[i].name;
                            matched = true;
                        }
                }
                if ( matched == true )
                {
                    if (expected_tokens > 2 )
                        state = Equals;
                    else
                        state = Done; 
                }
                else
                    state = Done;
                break;
            case Equals:
                if (!strncmp(t,"=",5) )
                {
                    matched = true;
                    state = Value;
                }
                else
                {
                    state = Done;
                }
                
            break;
            case Value:
                state = Done;
                matched = true;
                the_value = atoi(t);
                break;
            case Done:
            default:
                state = Done;
                break;
        }
        
        token++;
    }while ( token < num_tokens && state != Done && matched == true );
    return matched;
}
unsigned int StringHandler::get_command(void)
{
    return the_command;
}
unsigned int StringHandler::get_attribute(void)
{
    return the_attribute;
}
unsigned int StringHandler::get_value(void)
{
    return the_value;
}
