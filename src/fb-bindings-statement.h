/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#ifndef SRC_FB_BINDINGS_STATEMENT_H_
#define SRC_FB_BINDINGS_STATEMENT_H_

#include "./fb-bindings.h"
#include "./fb-bindings-connection.h"
#include "./fb-bindings-fbeventemitter.h"


class FBStatement : public FBEventEmitter {
public:
 static Persistent<FunctionTemplate> constructor_template; 

 static void
 Initialize (v8::Handle<v8::Object> target);

protected:
 static Handle<Value>
 New (const Arguments& args);
 
 static Handle<Value>
 ExecSync (const Arguments& args);
 
 static Handle<Value>
 Exec (const Arguments& args);
 
  
 FBStatement(XSQLDA *insqlda, XSQLDA *outsqlda, isc_stmt_handle *astmtp, Connection* aconn); 
 ~FBStatement(); 

private: 
 Connection* conn;
 XSQLDA *in_sqlda;
 XSQLDA *out_sqlda;
 isc_stmt_handle stmt;
 bool retres;
 
 ISC_STATUS_ARRAY status;
 char err_message[MAX_ERR_MSG_LEN];
 char cursor[50];

};


#endif
