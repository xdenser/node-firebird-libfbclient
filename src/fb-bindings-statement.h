/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#ifndef SRC_FB_BINDINGS_STATEMENT_H_
#define SRC_FB_BINDINGS_STATEMENT_H_
#define BUILDING_NODE_EXTENSION 1

#include "./fb-bindings.h"
//#include "./fb-bindings-connection.h"
//#include "./fb-bindings-fbeventemitter.h"
#include "./fb-bindings-fbresult.h"

static Nan::Persistent<String> error_symbol;
static Nan::Persistent<String> result_symbol;

class FBStatement:public FBResult{
public:
 static Nan::Persistent<FunctionTemplate> constructor_template; 

 static void
 Initialize (v8::Local<v8::Object> target);

protected:
 static NAN_METHOD(New);

 void InstExecSync(const Nan::FunctionCallbackInfo<v8::Value>& info, Transaction* transaction, int firstArg);
 static NAN_METHOD(ExecSync);
 static NAN_METHOD(ExecInTransSync);
 
 struct exec_request {
     FBStatement *statement;
	 Transaction *trans;
	 bool result;
 };
 
 static void EIO_After_Exec(uv_work_t *req);
    
 static void EIO_Exec(uv_work_t *req);
 
 void InstExec(const Nan::FunctionCallbackInfo<v8::Value>& info, Transaction* transaction, int firstArg);
 static NAN_METHOD(Exec);
 static NAN_METHOD(ExecInTrans);
 
  
 FBStatement(XSQLDA *insqlda, XSQLDA *outsqlda, isc_stmt_handle *astmtp, Connection* aconn); 
 ~FBStatement(); 

private: 
 XSQLDA *in_sqlda;
 int	statement_type;
 bool retres;
 
 void finished() override {};
 char cursor[50];

};


#endif
