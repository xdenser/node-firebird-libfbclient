/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

/**
 * Include headers
 *
 * @ignore
 */
#include "./fb-bindings-connection.h"
#include "./fb-bindings-fbresult.h"
#include "./fb-bindings-fbeventemitter.h"

/**
 * Init V8 structures
 *
 * Classes to populate in JavaScript:
 *
 * * Connection
 * * FBResult
 * *
 */
char * ErrorMessage(const ISC_STATUS *pvector, char *err_msg, int max_len)
{
    err_msg[0] = 0;
    char s[512], *p, *t;
    t = err_msg;
    while(fb_interpret(s,sizeof(s),&pvector)){
      for(p=s;(*p)&&((t-err_msg)<max_len);){ *t++ = *p++; }
      if((t-err_msg+1)<max_len) *t++='\n';
    };
    if((t-err_msg)<max_len) *t = 0;
    return err_msg;
}

extern "C" void
init (Handle<Object> target)
{
  HandleScope scope;

  start_async_symbol = NODE_PSYMBOL("fbStartAsync");
  stop_async_symbol = NODE_PSYMBOL("fbStopAsync");

  FBResult::Initialize(target);
  Connection::Initialize(target);
}
