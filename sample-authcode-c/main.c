//
//  main.c
//  sample-authcode-c
//
//  Copyright (c) 2013 TradeStation Technologies. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <curl/curl.h>
#include "jsmn.h"

#define API_KEY "place api key here"
#define API_SECRET "place api secret here"
#define SIM_ENVIRONMENT "https://sim.api.tradestation.com/v2"
#define REDIRECT_URI "http://www.tradestation.com"
#define BUFFER_SIZE 32768
#define JSON_TOKENS 256

typedef struct
{
    size_t len;     // current length of buffer (used bytes)
    size_t limit;   // maximum length of buffer (allocated)
    uint8_t *data;  // insert bytes here
} buf_t;

typedef struct
{
    char *access_token;
    char *refresh_token;
} Token;

// prototypes
char *getAuthCode( void );
char *getAccessToken( const char *authCode );
void getQuote( const char *accessToken );
buf_t * buf_size( buf_t *buf, size_t len );
jsmntok_t * json_tokenize( char *json );

int main( void )
{
    char *authCode;
    char *accessToken;
    
    // initialize curl
    curl_global_init( CURL_GLOBAL_DEFAULT );
    
    authCode = getAuthCode();
    accessToken = getAccessToken( authCode );
    
    printf( "Access Token: %s", accessToken );
    
    getQuote( accessToken );
    
    free( authCode );
    authCode = NULL;
    accessToken = NULL;
    curl_global_cleanup();
}

char *getAuthCode( void )
{
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
    
    // Handle input authcode of varying length: http://www.mkyong.com/c/how-to-handle-unknow-size-user-input-in-c/
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

char * getAccessToken( const char *authCode )
{
    // function prototypes
    char * buf_tostr( buf_t *buf );
    size_t fetch_data( void *buffer, size_t size, size_t nmemb, void *userp );

    CURL *curl = curl_easy_init();
    char *json;
    char *KEYS[] = { "access_token" };
    char *accessToken;
    
    if ( curl ) {
        curl_easy_setopt( curl, CURLOPT_URL, SIM_ENVIRONMENT"/security/authorize");
        
        buf_t *buf = buf_size( NULL, BUFFER_SIZE );
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, fetch_data );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, buf );
        curl_easy_setopt( curl, CURLOPT_USERAGENT, "webapi-sample-c" );
        
        struct curl_slist *headers = curl_slist_append( NULL, "Accept: application/json" );
        curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );
        
        char buffer [1024];
        sprintf( buffer, "%s=%s&%s=%s&%s=%s&%s=%s&%s=%s",
                "grant_type", "authorization_code",
                "code", authCode,
                "client_id", API_KEY,
                "redirect_uri", REDIRECT_URI,
                "client_secret", API_SECRET );
        curl_easy_setopt( curl, CURLOPT_POST, 1 );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDS, buffer );
        
        CURLcode result = curl_easy_perform( curl );
        
        if ( CURLE_OK != result ) {
            fprintf( stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror( result ) );
        }
        
        curl_easy_cleanup( curl );
        curl_slist_free_all( headers );
        
        json = buf_tostr( buf );
        free( buf );
    } else {
        fprintf( stderr, "FAIL: %s\n", "CURL did not initialize" );
        json = NULL;
    }
    
    // parse json: http://alisdair.mcdiarmid.org/2012/08/14/jsmn-example.html
    if ( NULL != json ) {
        jsmntok_t *tokens = json_tokenize( json );
        
        typedef enum { START, KEY, PRINT, SKIP, STOP } parse_state;
        parse_state state = START;
        
        size_t object_tokens = 0;
        
        for ( size_t i = 0, j = 1; j > 0; i++, j-- ) {
            jsmntok_t *t = &tokens[ i ];
            
            if ( JSMN_ARRAY == t->type || JSMN_OBJECT == t->type ) {
                j += t->size;
            }
            
            switch ( state ) {
                case START:
                    state = KEY;
                    object_tokens = t->size;
                    
                    if ( 0 == object_tokens ) {
                        state = STOP;
                    }
                    
                    break;
                    
                case KEY:
                    object_tokens--;
                    
                    state = SKIP;
                    
                    for ( size_t i = 0; i < sizeof( KEYS ) / sizeof( char * ); i++) {
                        if ( strncmp( json + t->start, KEYS[ i ], t->end - t->start ) == 0
                            && strlen( KEYS[ i ] ) == ( size_t ) ( t->end - t->start ) ) {
                            state = PRINT;
                            break;
                        }
                    }
                    
                    break;
                    
                case SKIP:
                    object_tokens--;
                    state = KEY;
                    
                    if ( 0 == object_tokens ) {
                        state = STOP;
                    }
                    
                    break;
                    
                case PRINT:
                    json[ t->end ] = '\0';
                    accessToken = json + t->start;
                    
                    object_tokens--;
                    state = KEY;
                    
                    if ( 0 == object_tokens ) {
                        state = STOP;
                    }
                    
                    break;
                    
                case STOP:
                    // Just consume the tokens
                    break;
            }
        }
        
        return accessToken;
    } else {
        return "FAIL: Unable to parse json";
    }
}

void getQuote( const char *accessToken )
{
    CURL *curl = curl_easy_init();
    
    if ( curl ) {
        char buffer [1024];
        sprintf( buffer, SIM_ENVIRONMENT"/data/quote/msft?oauth_token=%s", accessToken );
        curl_easy_setopt( curl, CURLOPT_URL, buffer );
        
        puts( "\n\nGot Data Quote:\n\n" );
        CURLcode result = curl_easy_perform( curl );
        
        if ( CURLE_OK != result ) {
            fprintf( stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror( result ) );
        }
        
        curl_easy_cleanup( curl );
    }
}

char * buf_tostr( buf_t *buf )
{
    char *str = malloc( buf->len + 1 );
    
    memcpy( str, buf->data, buf->len );
    str[ buf->len ] = '\0';
    
    return str;
}

buf_t * buf_size( buf_t *buf, size_t len )
{
    if ( NULL == buf ) {
        buf = malloc( sizeof( buf_t ) );
        buf->data = NULL;
        buf->len = 0;
    }
    
    buf->data = realloc( buf->data, len );
    if ( buf->len > len )
        buf->len = len;
    buf->limit = len;
    
    return buf;
}

static size_t fetch_data( void *buffer, size_t size, size_t nmemb, void *userp )
{
    void buf_concat( buf_t *dst, uint8_t *src, size_t len ); // function prototype
    
    buf_t *buf = (buf_t *) userp;
    size_t total = size * nmemb;
    
    if ( buf->limit - buf->len < total ) {
        buf = buf_size( buf, buf->limit + total );
    }
    
    buf_concat( buf, buffer, total );
    
    return total;
}

void buf_concat( buf_t *dst, uint8_t *src, size_t len )
{
    for ( size_t i = 0; i < len; i++ ) {
        dst->data[ dst->len++ ] = src[ i ];
    }
}

jsmntok_t * json_tokenize( char *json )
{
    jsmn_parser parser;
    jsmn_init( &parser );
    
    unsigned int n = JSON_TOKENS;
    jsmntok_t *tokens = malloc( sizeof( jsmntok_t ) * n );
    
    int ret = jsmn_parse( &parser, json, tokens, n );
    
    while ( JSMN_ERROR_NOMEM == ret ) {
        n = n * 2 + 1;
        tokens = realloc( tokens, sizeof( jsmntok_t ) * n );
        ret = jsmn_parse( &parser, json, tokens, n );
    }
    
    return tokens;
}