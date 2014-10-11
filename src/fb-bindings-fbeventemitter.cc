/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#define BUILDING_NODE_EXTENSION 1
#include "./fb-bindings-fbeventemitter.h"

Persistent<FunctionTemplate> FBEventEmitter::constructor_template;

void FBEventEmitter::Initialize(v8::Handle<v8::Object> target)
  {
    
    Local<FunctionTemplate> t = NanNew<FunctionTemplate>();
    
    NanAssignPersistent(constructor_template,t);
    t->SetClassName(NanNew<String>("FBEventEmitter"));
           
    target->Set(NanNew<String>("FBEventEmitter"),  t->GetFunction());
  }
 
 
void FBEventEmitter::Emit(Handle<String> event, int argc, Handle<Value> argv[])
  {
	NanScope();;
    Handle<Value> argv1[11];
    if(argc>10) NanThrowError("Cant process more than 10 arguments");
    argv1[0] = event;
    for(int i=0;i<argc;i++) argv1[i+1] = argv[i];
    NanMakeCallback(handle(),"emit",argc+1,argv1);
    //node::MakeCallback(handle_,"emit",argc+1,argv1);
  }  

void FBEventEmitter::start_async()
  {
    in_async = true;
    Emit(NanNew("fbStartAsync"), 0, NULL);
  }
  
void FBEventEmitter::stop_async()
  {
    in_async = false;
    Emit(NanNew("fbStopAsync"), 0, NULL);
  }

FBEventEmitter::FBEventEmitter () : ObjectWrap () 
  {
    in_async = false;
   // Handle<Value> argv[1];
   // printf("in init\n");
   // node::MakeCallback(handle_,"init",0,argv);
  }

/*Handle<Value>
  FBEventEmitter::InAsyncGetter(Local<String> property,
                                const AccessorInfo &info) */

NAN_GETTER(FBEventEmitter::InAsyncGetter)
  {
    NanScope();
    FBEventEmitter *fbee = ObjectWrap::Unwrap<FBEventEmitter>(args.This());
    NanReturnValue(NanNew<Boolean>(fbee->in_async));
  }
