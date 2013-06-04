//
//  main.c
//  sample-authcode-c
//
//  Created by John Jelinek on 6/4/13.
//  Copyright (c) 2013 TradeStation Technologies. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

#define API_KEY "place api key here"
#define API_SECRET "place api secret here"
#define SIM_ENVIRONMENT "https://sim.api.tradestation.com/v2"
#define REDIRECT_URI "http://www.tradestation.com"

// prototypes
char *getAuthCode( void );
CURLcode getAccessToken( CURL **curl, const char *authCode );

int main( void )
{
    char *authCode;
    CURL *curl;
    CURLcode accessToken;
    
    // initialize curl
    curl_global_init( CURL_GLOBAL_DEFAULT );
    curl = curl_easy_init();
    
    authCode = getAuthCode();
    printf( "\nAuth Code: %s\n\n", authCode );
    
    accessToken = getAccessToken( curl, authCode );
    
    free( authCode );
    authCode = NULL;
    curl_global_cleanup();
}

char *getAuthCode( void )
{
    /* http://www.mkyong.com/c/how-to-handle-unknow-size-user-input-in-c/ */
    unsigned int lenMax = 128;
    unsigned int currentSize = 0;
    
    char *sPtr = malloc( lenMax );
    currentSize = lenMax;
    
    printf( "%s", "Go here and login:\n"
           SIM_ENVIRONMENT
           "/authorize?client_id="
           API_KEY
           "&response_type=code&redirect_uri="
           REDIRECT_URI
           "\nPaste in the authorization code here:\n");
    
    if ( NULL != sPtr ) {
        int c = EOF;
        unsigned int i = 0;
        // accept user input until hit enter or end of file
        while ( ( c = getchar() ) != '\n' && EOF != c ) {
            sPtr[ i++ ] = (char)c;
            
            // if maximum size is reached then realloc
            if ( i == currentSize ) {
                currentSize = i + lenMax;
                sPtr = realloc( sPtr, currentSize );
            }
        }
        
        sPtr[ i ] = '\0';
    }
    
    return sPtr;
}

CURLcode getAccessToken( CURL **curl, const char *authCode )
{
    CURLcode result;
    
    if ( curl ) {
        curl_easy_setopt( curl, CURLOPT_URL, "http://www.google.com");
        
        result = curl_easy_perform( curl );
        
        if ( result != CURLE_OK ) {
            fprintf( stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror( result ) );
        }
        
        curl_easy_cleanup( curl );
    }
    else {
        exit( 0 );
    }
    
    return result;
}
