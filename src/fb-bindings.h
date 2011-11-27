/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#ifndef SRC_FB_BINDINGS_H_
#define SRC_FB_BINDINGS_H_

#include <v8.h>
#include <node.h>
#include <ibase.h>

#define MAX_ERR_MSG_LEN 1024
#define ERR_MSG(obj,class) \
String::New(ErrorMessage(obj->status,obj->err_message,sizeof(obj->err_message)))

#define ERR_MSG_STAT(status,class) \
String::New(ErrorMessage(status,class::err_message,sizeof(class::err_message)))


#define REQ_EXT_ARG(I, VAR) \
if (args.Length() <= (I) || !args[I]->IsExternal()) \
return ThrowException(Exception::TypeError( \
String::New("Argument " #I " invalid"))); \
Local<External> VAR = Local<External>::Cast(args[I]);

char * ErrorMessage(const ISC_STATUS *pvector, char *err_msg, int max_len);

class Connection;

#endif

