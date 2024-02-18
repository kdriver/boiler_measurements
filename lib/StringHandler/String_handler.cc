#include "StringHandler.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <Arduino.h>



StringHandler::StringHandler(const char *text,unsigned int num_cmds, CommandSet *cmds,unsigned int num_attr, AttributeSet *attrs)
{
    unsigned long len;
    unsigned int i = 0;
    // copy the string into an internal buffer
    if ( strlen(text) < max_input_length )
    {
        Serial.println("len of cli command is "+String(strlen(text)));
        strcpy(string_copy,text);
    }
    len = strlen(string_copy);
    for (i=0;i<len;i++)
    {
        string_copy[i] = toupper(string_copy[i]);
    }
    string_copy[i]=0;
    num_tokens =0;
    num_commands = num_cmds;
    commands = cmds;
    num_attributes = num_attr;
    attributes = attrs;
    the_value = 0;
    the_f_value = 0.0;
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
    // Serial.println("tokenise ^" + String(string_copy) +"^");
    dest_iterator = &tokens[token][0];
    // skip over any leading white space
    do {
            switch(*input_iterator)
            {
                case '\n':
                case ' ':
                    // Serial.println("tokenise: whitespace");

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
                    // Serial.println("tokenise: case null");

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
                    // Serial.println("tokenise: default(" + String(*input_iterator)+")");
                    now = Collecting;
                    *dest_iterator = *input_iterator;
                    dest_iterator = dest_iterator + 1 ;
                    input_iterator = input_iterator + 1;
                    if ( *input_iterator == 0 )
                    {
                        // Serial.println("tokenise: null");
                        *dest_iterator = 0;
                        token = token + 1 ;
                    }
                    break;
            }
          
    }  while ((*input_iterator != 0 ) && (token < 5 ));
    
    num_tokens = token;
    // Serial.println(" found " + String(num_tokens) + " tokens");

    
    return token;
}
const char *StringHandler::get_token(unsigned int token)
{
    // forst token is at 0 index
    if ( token < num_tokens )
        return &tokens[token][0];
    else
        return "Invalid token";
}

bool StringHandler::validate(void)
{
    unsigned int token=0;
    char *t;
    bool matched;
    
    Grammar state;
    state = CommandPrimitive;
    unsigned int i;
    int expected_tokens=0;
    
    
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
                if ( strstr(t,".") != NULL )
                    the_f_value = atof(t);
                
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
float StringHandler::get_f_value(void)
{
    return the_f_value;
}
