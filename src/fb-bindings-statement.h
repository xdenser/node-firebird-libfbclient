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

static Persistent<String> error_symbol;
static Persistent<String> result_symbol;

class FBStatement:public FBResult{
public:
 static Persistent<FunctionTemplate> constructor_template; 

 static void
 Initialize (v8::Handle<v8::Object> target);

protected:
 static Handle<Value>
 New (const Arguments& args);
 
 static Handle<Value>
 ExecSync (const Arguments& args);
 
 struct exec_request {
     FBStatement *statement;
	 bool result;
 };
 
 static void EIO_After_Exec(uv_work_t *req);
    
 static void EIO_Exec(uv_work_t *req);
 
 static Handle<Value>
 Exec (const Arguments& args);
 
  
 FBStatement(XSQLDA *insqlda, XSQLDA *outsqlda, isc_stmt_handle *astmtp, Connection* aconn); 
 ~FBStatement(); 

private: 
 XSQLDA *in_sqlda;
 int	statement_type;
 bool retres;
 
 char cursor[50];

};


#endif
