/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_FBRESULT_H_
#define SRC_FB_BINDINGS_FBRESULT_H_


#define BUILDING_NODE_EXTENSION 1
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctime> 
//#include <eio.h>
#include "./fb-bindings.h"
#include "./fb-bindings-fbeventemitter.h"
#include "./fb-bindings-blob.h"
#include "./fb-bindings-connection.h"


class FBResult : public FBEventEmitter {

public:

 static Nan::Persistent<FunctionTemplate> constructor_template;
 
 static void
   Initialize (v8::Local<v8::Object> target);
  
// bool process_result(XSQLDA **sqldap, isc_stmt_handle *stmtp, Local<Array> res);
  
 static bool prepare_sqlda(XSQLDA *sqlda);
 static void clean_sqlda(XSQLDA *sqlda);
 static bool clone_sqlda(XSQLDA *src_sqlda,XSQLDA **dest_sqlda);
 static void set_params(XSQLDA *sqlda, Nan::NAN_METHOD_ARGS_TYPE info, int firstArg);
 Local<Object> getCurrentRow(bool asObject);
   
protected:  
  static NAN_METHOD(New);
  
    
  static Local<Value> 
  GetFieldValue(XSQLVAR *var, Connection* conn);

  static NAN_METHOD(FetchSync);

  struct fetch_request {
     Nan::Callback *rowCallback;
     Nan::Callback *eofCallback;
     FBResult *res;
     int rowCount;
     ISC_STATUS fetchStat;
     bool rowAsObject;
	 bool result;
  };
  
  static void EIO_After_Fetch(uv_work_t *req);

  static void EIO_Fetch(uv_work_t *req);
  
  static NAN_METHOD(Fetch);
  
  FBResult(XSQLDA *asqldap, isc_stmt_handle *astmtp, Connection *conn); 
  
  virtual ~FBResult();
  
  ISC_STATUS_ARRAY status;
  char err_message[MAX_ERR_MSG_LEN];
  XSQLDA *sqldap;
  isc_stmt_handle stmt;
  Connection *connection;
  static Nan::AsyncResource asyncResource;
  
private:
  static const double dscales[19];
  virtual void finished();
  void releaseStatement(); 
    
};

#endif
