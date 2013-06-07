# sample-authcode-c

This sample application uses C to authenticate with the TradeStation WebAPI by the Authorization Code Grant Type. The user will be directed to TradeStation's login page to capture credentials. After a user logs in with their TradeStation account, an authorization code is returned which is exchanged for an access token to be used for subsequent WebAPI calls.

## Configuration
Modify the following #define identifiers in main.c with your appropriate values:

    #define API_KEY "place your key here"
    #define API_SECRET "place your secret here"
    #define REDIRECT_URI "place your webapp URL here"

## Build Instructions
* Open the project in XCode
* Build and Run

## TODO
* Create Makefile
* Create Build Steps for Linux/Windows

## Troubleshooting

If there are any problems, open an issue and we'll take a look! You can also give us feedback by e-mailing webapi@tradestation.com.
