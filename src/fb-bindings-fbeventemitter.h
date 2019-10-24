/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_FBEVENTEMITTER_H_
#define SRC_FB_BINDINGS_FBEVENTEMITTER_H_
#define BUILDING_NODE_EXTENSION 1

#include "./fb-bindings.h"

using namespace node;
using namespace v8;


class FBEventEmitter: public Nan::ObjectWrap {
public: 
  static void Initialize(v8::Local<v8::Object> target);
  static Nan::Persistent<v8::FunctionTemplate> constructor_template;
  void Emit(Local<String> event, int argc, Local<Value> argv[]);

protected:
  void start_async();

  void stop_async();

  FBEventEmitter ();

  /*static Local<Value>
  InAsyncGetter(Local<String> property,
                      const AccessorInfo &info);
  */
  static NAN_GETTER(InAsyncGetter);
  
  static Nan::AsyncResource asyncResource;

private:
  bool in_async;
};

#endif
