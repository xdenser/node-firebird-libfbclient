/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_CONNECTION_H_
#define SRC_FB_BINDINGS_CONNECTION_H_
#define BUILDING_NODE_EXTENSION 1

#define MAX_USERNAME_LENGTH 31
#define MAX_ROLENAME_LENGTH 31
#define MAX_PASSWORD_LENGTH 8 

#include <uv.h>
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
  
  bool process_statement(XSQLDA **sqldap, char *query, isc_stmt_handle *stmtp, int *statement_type);
  
  bool prepare_statement(XSQLDA **insqlda, XSQLDA **outsqlda, char *query, isc_stmt_handle *stmtp);
 
  bool commit_transaction();
    
  bool rollback_transaction();
  
  bool start_transaction();
  
  void doref();
  
  void dounref();
  
  isc_db_handle get_DB_Handle();
  isc_tr_handle get_Def_Tr_Handle();
  
 
 protected:
 
  static NAN_METHOD(New);
  static NAN_METHOD(ConnectSync);
 
  struct connect_request {
     Nan::Callback *callback;
     Connection *conn;
     String::Utf8Value *Database;
     String::Utf8Value *User;
     String::Utf8Value *Password;
     String::Utf8Value *Role;
     bool res;
  };
  
  static void EIO_After_Connect(uv_work_t *req);
  
  static void EIO_Connect(uv_work_t *req);

  static NAN_METHOD(Connect);
  static NAN_METHOD(Disconnect);
  
  static NAN_GETTER(ConnectedGetter);
  static NAN_GETTER(InTransactionGetter);
  
  static NAN_METHOD(CommitSync);
  static NAN_METHOD(RollbackSync);
  static NAN_METHOD(StartSync);
    

  enum TransReqType {
    rCommit,
    rRollback,
    rStart
  };
    
  struct transaction_request {
	 Nan::Callback *callback;
     Connection *conn;
     TransReqType type;
     bool result;
  };

  static void EIO_After_TransactionRequest(uv_work_t *req);
  
  static void EIO_TransactionRequest(uv_work_t *req);
  
  static NAN_METHOD(Commit);
  static NAN_METHOD(Rollback);
  static NAN_METHOD(Start);
  static NAN_METHOD(QuerySync);

  struct query_request {
	 Nan::Callback *callback;
     Connection *conn;
     String::Utf8Value *Query;
     XSQLDA *sqlda;
     isc_stmt_handle stmt;
     int statement_type;
	 bool result;
  };
  
  static void EIO_After_Query(uv_work_t *req);
    
  static void EIO_Query(uv_work_t *req);
  
  static NAN_METHOD(Query);
  
  static NAN_METHOD(addEvent);
  static NAN_METHOD(deleteEvent);
  static NAN_METHOD(PrepareSync);
  static NAN_METHOD(NewBlobSync);

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
