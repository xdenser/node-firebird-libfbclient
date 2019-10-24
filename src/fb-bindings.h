/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#ifndef SRC_FB_BINDINGS_H_
#define SRC_FB_BINDINGS_H_

// #include <v8.h>
#include <nan.h>
#include <node.h>
#include <ibase.h>

#if NODE_MODULE_VERSION < NODE_10_0_MODULE_VERSION
#	define FB_MAYBE_NEED_ISOLATE
#else
#	define FB_MAYBE_NEED_ISOLATE Isolate::GetCurrent(),
#endif

#define MAX_ERR_MSG_LEN 1024
#define ERR_MSG(obj,class) \
Nan::New<String>(ErrorMessage(obj->status,obj->err_message,sizeof(obj->err_message))).ToLocalChecked()

#define ERR_MSG_STAT(status,class) \
Nan::New<String>(ErrorMessage(status,class::err_message,sizeof(class::err_message))).ToLocalChecked()


#define REQ_EXT_ARG(I, VAR) \
if (info.Length() <= (I) || !info[I]->IsExternal()) \
return Nan::ThrowTypeError( \
"Argument " #I " invalid"); \
Local<External> VAR = Local<External>::Cast(info[I]);

char * ErrorMessage(const ISC_STATUS *pvector, char *err_msg, int max_len);

class Connection;
class Transaction;

#endif

