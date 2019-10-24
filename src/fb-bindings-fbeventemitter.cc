/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#define BUILDING_NODE_EXTENSION 1
#include "./fb-bindings-fbeventemitter.h"

Nan::Persistent<FunctionTemplate> FBEventEmitter::constructor_template;

Nan::AsyncResource FBEventEmitter::asyncResource("FBEventEmitter");

void FBEventEmitter::Initialize(v8::Local<v8::Object> target)
  {
    
    Local<FunctionTemplate> t = Nan::New<FunctionTemplate>();
    
    constructor_template.Reset(t);
    t->SetClassName(Nan::New<String>("FBEventEmitter").ToLocalChecked());
           
    Nan::Set(target, Nan::New<String>("FBEventEmitter").ToLocalChecked(),  Nan::GetFunction(t).ToLocalChecked());
  }
 
 
void FBEventEmitter::Emit(Local<String> event, int argc, Local<Value> argv[])
  {
    if(!persistent().IsEmpty()) {
    	Nan::HandleScope scope;
	    Local<Value> argv1[11];
	    if(argc>10) Nan::ThrowError("Cant process more than 10 arguments");
	    argv1[0] = event;
	    for(int i=0;i<argc;i++) argv1[i+1] = argv[i];
		asyncResource.runInAsyncScope(this->handle(), Nan::New("emit").ToLocalChecked(), argc+1, argv1);
    }
  }  

void FBEventEmitter::start_async()
  {
	  Nan::HandleScope scope;
	  in_async = true;
    Emit(Nan::New("fbStartAsync").ToLocalChecked(), 0, NULL);
  }
  
void FBEventEmitter::stop_async()
  {
	Nan::HandleScope scope;
	in_async = false;
    Emit(Nan::New("fbStopAsync").ToLocalChecked(), 0, NULL);
  }

FBEventEmitter::FBEventEmitter () : Nan::ObjectWrap ()
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
    Nan::HandleScope scope;
    FBEventEmitter *fbee = Nan::ObjectWrap::Unwrap<FBEventEmitter>(info.This());
    info.GetReturnValue().Set(Nan::New<Boolean>(fbee->in_async));
  }
