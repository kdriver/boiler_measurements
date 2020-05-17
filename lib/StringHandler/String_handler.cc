#include "StringHandler.h"
#include <string.h>

StringHandler::StringHandler(const char *text)
{
    // copy the string into an internal buffer
    if ( strlen(text) < max_input_length )
    {
        strcpy(string_copy,text);
    }

}

StringHandler::~StringHandler()
{

}

unsigned int StringHandler::tokenise(void)
{
    unsigned int token=0;
    char *input_iterator = string_copy;
    char *dest_iterator;


    // skip over any leading white space
    do {
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
    }  while ((*input_iterator != 0 ) && (token < 5 ));

    return token;
}
const char *StringHandler::get_token(unsigned int token)
{
    // forst token is at 0 index
    return &tokens[token][0];
}

