/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_CONNECTION_H_
#define SRC_FB_BINDINGS_CONNECTION_H_
#define BUILDING_NODE_EXTENSION 1


#include <uv-private/ev.h>
#include "./fb-bindings.h"
#include "./fb-bindings-fbeventemitter.h"
#include "./fb-bindings-eventblock.h"
#include "./fb-bindings-blob.h"
#include <string.h>
#include <stdlib.h>

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
  
  bool prepare_statement(XSQLDA **insqlda, XSQLDA **outsqlda, char *query, isc_stmt_handle *stmtp);
 
  bool commit_transaction();
    
  bool rollback_transaction();
  
  bool start_transaction();
  
  void doref();
  
  void dounref();
  
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
  
  static void EIO_After_Connect(uv_work_t *req);
  
  static void EIO_Connect(uv_work_t *req);
  
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
  
  static Handle<Value>
  StartSync (const Arguments& args);
  
  enum TransReqType {
    rCommit,
    rRollback,
    rStart
  };
    
  struct transaction_request {
     Persistent<Function> callback;
     Connection *conn;
     TransReqType type;
     bool result;
  };

  static void EIO_After_TransactionRequest(uv_work_t *req);
  
  static void EIO_TransactionRequest(uv_work_t *req);
  
  static Handle<Value>
  Commit (const Arguments& args);
  
  static Handle<Value>
  Rollback (const Arguments& args);
  
  static Handle<Value>
  Start (const Arguments& args);
  
  static Handle<Value>
  QuerySync(const Arguments& args);

  struct query_request {
     Persistent<Function> callback;
     Connection *conn;
     String::Utf8Value *Query;
     XSQLDA *sqlda;
     isc_stmt_handle stmt;
	 bool result;
  };
  
  static void EIO_After_Query(uv_work_t *req);
    
  static void EIO_Query(uv_work_t *req);
  
  static Handle<Value>
  Query(const Arguments& args);

  static Handle<Value>
  addEvent(const Arguments& args);

  static Handle<Value>
  deleteEvent(const Arguments& args);
  
  static Handle<Value>
  PrepareSync (const Arguments& args);
  
  static Handle<Value>
  NewBlobSync (const Arguments& args);
  
  static time_t 
  get_gmt_delta();

   Connection (); 

  ~Connection ();
 
 private:

  ISC_STATUS_ARRAY status;
  event_block* fb_events;
  bool connected; 
  
//  bool in_async;
  char err_message[MAX_ERR_MSG_LEN];
  

};

#endif
