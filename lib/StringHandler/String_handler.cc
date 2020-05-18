#include "StringHandler.h"
#include <string.h>
#include <ctype.h>


StringHandler::StringHandler(const char *text)
{
    unsigned int len;
    // copy the string into an internal buffer
    if ( strlen(text) < max_input_length )
    {
        strcpy(string_copy,text);
    }
    len = strlen(string_copy);
    for (unsigned int i=0;i<len;i++)
        string_copy[i] = toupper(string_copy[i]);
    
    num_tokens =0;

}

StringHandler::~StringHandler()
{

}
/*
 while ( *input_iterator == ' ' || *input_iterator == '\n' )
           input_iterator = input_iterator + 1;
 
       dest_iterator = &tokens[token][0];
       do
       {
           *dest_iterator = *input_iterator;
           dest_iterator = dest_iterator + 1 ;
           input_iterator = input_iterator + 1;
           if ( input_iterator[-1] == '=' )
               break;
       } while ( *input_iterator != ' '  && *input_iterator != '=' && *input_iterator != '\n' ) ;
       *dest_iterator = 0;
       token = token + 1;
 */
 
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
    bool ok = true;
    char *t;
    
    Grammar state;
    state = CommandPrimitive;
    
    do {
        t = tokens[token];
        switch (state)
        {
            case CommandPrimitive:
                if (!strncmp(t,"GET",5))
                {
                    command = Get;
                    if ( num_tokens !=  2)
                        ok = false;
                }
                else if (!strncmp(t,"SET",5) )
                {
                    command = Set;
                    if ( num_tokens != 4 )
                        ok = false;
                }
                else
                    ok = false;
                state = VariablePrimitive;
            break;
            case VariablePrimitive:
                if (!strncmp(t,"OFFTHRESHOLD",15))
                    variable = off_threshold;
                else if (!strncmp(t,"ONTHRESHOLD",15) )
                    variable = on_threshold;
                else
                        ok = false;
                state = Equals;
                break;
            case Equals:
                if (strncmp(t,"=",5) )
                    ok = false;
                state = Value;
            break;
            case Value:
                state = Done;
                break;
            case Done:
            default:
                state = Done;
                break;
        }
        
        token++;
    }while ( token < num_tokens && state != Done && ok == true );
    return ok;
}

