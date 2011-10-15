/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#include <node.h>
#include "./fb-bindings-fbeventemitter.h"

Persistent<FunctionTemplate> FBEventEmitter::constructor_template;

void FBEventEmitter::Initialize(v8::Handle<v8::Object> target)
  {
    start_async_symbol = NODE_PSYMBOL("fbStartAsync");
    stop_async_symbol = NODE_PSYMBOL("fbStopAsync");
    
    Local<FunctionTemplate> t = FunctionTemplate::New();
    
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->SetClassName(String::NewSymbol("FBEventEmitter"));
           
    target->Set(String::NewSymbol("FBEventEmitter"),  constructor_template->GetFunction());
  }
 
 
void FBEventEmitter::Emit(Handle<String> event, int argc, Handle<Value> argv[])
  {
    HandleScope scope;
    Handle<Value> argv1[11];
    if(argc>10) ThrowException(Exception::Error(String::New("Cant process more than 10 arguments")));
    argv1[0] = event;
    for(int i=0;i<argc;i++) argv1[i+1] = argv[i];
    node::MakeCallback(handle_,"emit",argc+1,argv1);
  }  

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

FBEventEmitter::FBEventEmitter () : ObjectWrap () 
  {
    in_async = false;
   // Handle<Value> argv[1];
   // printf("in init\n");
   // node::MakeCallback(handle_,"init",0,argv);
  }

Handle<Value>
  FBEventEmitter::InAsyncGetter(Local<String> property,
                                const AccessorInfo &info) 
  {
    HandleScope scope;
    FBEventEmitter *fbee = ObjectWrap::Unwrap<FBEventEmitter>(info.Holder());
    return scope.Close(Boolean::New(fbee->in_async));
  }
