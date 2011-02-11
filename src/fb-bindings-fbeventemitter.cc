/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#include "./fb-bindings-fbeventemitter.h"

void FBEventEmitter::start_async()
  {
    in_async = true;
    Emit(start_async_symbol, 0, NULL);  
  }
  
void FBEventEmitter::stop_async()
  {
    in_async = false;
    Emit(stop_async_symbol, 0, NULL);  
  }

FBEventEmitter::FBEventEmitter () : EventEmitter () 
  {
    in_async = false;
  }

Handle<Value>
  FBEventEmitter::InAsyncGetter(Local<String> property,
                                const AccessorInfo &info) 
  {
    HandleScope scope;
    FBEventEmitter *fbee = ObjectWrap::Unwrap<FBEventEmitter>(info.Holder());
    return scope.Close(Boolean::New(fbee->in_async));
  }
