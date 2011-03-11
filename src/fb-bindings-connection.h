/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_CONNECTION_H_
#define SRC_FB_BINDINGS_CONNECTION_H_

#include <ibase.h>
#include <v8.h>
#include <node.h>
#include "./fb-bindings-fbeventemitter.h"
#include "./fb-bindings-eventblock.h"

using namespace v8;
using namespace node;

class Connection : public FBEventEmitter {
 public:
  isc_db_handle db;
  isc_tr_handle trans;

 
  static void
  Initialize (v8::Handle<v8::Object> target);
 
  bool Connect (const char* Database,const char* User,const char* Password,const char* Role);
 
  bool Close();
  
  bool process_statement(XSQLDA **sqldap, char *query, isc_stmt_handle *stmtp);
 
  bool commit_transaction();
    
  bool rollback_transaction();
  
  isc_db_handle get_DB_Handle();
  isc_tr_handle get_Def_Tr_Handle();
  
 
 protected:
 
 static Handle<Value>
  New (const Arguments& args);

  static Handle<Value>
  ConnectSync (const Arguments& args);
 
  struct connect_request {
     Persistent<Function> callback;
     Connection *conn;
     String::Utf8Value *Database;
     String::Utf8Value *User;
     String::Utf8Value *Password;
     String::Utf8Value *Role;
     bool res;
  };
  
  static int EIO_After_Connect(eio_req *req);
  
  static int EIO_Connect(eio_req *req);
  
  static Handle<Value>
  Connect (const Arguments& args);
  
  static Handle<Value>
  Disconnect(const Arguments& args);
  
  static Handle<Value> ConnectedGetter(Local<String> property,
                                      const AccessorInfo &info);
  
  static Handle<Value>
  InTransactionGetter(Local<String> property,
                      const AccessorInfo &info);
  
  static Handle<Value>
  CommitSync (const Arguments& args);
    
  static Handle<Value>
  RollbackSync (const Arguments& args);
  
  struct transaction_request {
     Persistent<Function> callback;
     Connection *conn;
     bool commit;
  };

  static int EIO_After_TransactionRequest(eio_req *req);
  
  static int EIO_TransactionRequest(eio_req *req);
  
  static Handle<Value>
  Commit (const Arguments& args);
  
  static Handle<Value>
  Rollback (const Arguments& args);
  
  static Handle<Value>
  QuerySync(const Arguments& args);

  struct query_request {
     Persistent<Function> callback;
     Connection *conn;
     String::Utf8Value *Query;
     XSQLDA *sqlda;
     isc_stmt_handle stmt;
  };
  
  static int EIO_After_Query(eio_req *req);
    
  static int EIO_Query(eio_req *req);
  
  static Handle<Value>
  Query(const Arguments& args);

  static Handle<Value>
  addEvent(const Arguments& args);

  static Handle<Value>
  deleteEvent(const Arguments& args);
  
  static time_t 
  get_gmt_delta();

   Connection (); 

  ~Connection ();
 
 private:

  ISC_STATUS_ARRAY status;
  event_block* fb_events;
  bool connected; 
  
  bool in_async;
  char err_message[MAX_ERR_MSG_LEN];
  

};

#endif
