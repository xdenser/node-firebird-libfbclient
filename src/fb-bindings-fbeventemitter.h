/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_FBEVENTEMITTER_H_
#define SRC_FB_BINDINGS_FBEVENTEMITTER_H_
/*
include <node.h>
include <v8.h>
*/
#include <node_events.h>

using namespace node;
using namespace v8;

static Persistent<String> start_async_symbol;
static Persistent<String> stop_async_symbol;

class FBEventEmitter : public EventEmitter {
public: 
  static void Init();

protected:
  void start_async();

  void stop_async();

  FBEventEmitter ();

  static Handle<Value>
  InAsyncGetter(Local<String> property,
                      const AccessorInfo &info);
private:
  bool in_async;
};

#endif
