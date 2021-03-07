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
#define MAX_PASSWORD_LENGTH 20 
#define PROP_LC_CTYPE "lc_ctype"
#define PROP_LC_CTYPE_DECODE "lc_ctype_decode"

#include <uv.h>
#include "./fb-bindings.h"
#include "./fb-bindings-fbeventemitter.h"
#include "./fb-bindings-eventblock.h"
#include "./fb-bindings-blob.h"
#include "./fb-bindings-transaction.h"
#include <string.h>
#include <stdlib.h>

using namespace v8;
using namespace node;

class Connection : public FBEventEmitter {

 public:
  isc_db_handle db;
  Transaction* def_trans;
  char *lc_type = const_cast<char *>("UTF8");
  bool isUTF8lctype = true;
   
  static void
  Initialize (v8::Local<v8::Object> target);
 
  bool Connect (const char* Database,const char* User,const char* Password,const char* Role, const char* lc_type);
 
  bool Close();
  
  bool process_statement(XSQLDA **sqldap, char *query, isc_stmt_handle *stmtp, int *statement_type, Transaction *atr);
  
  bool prepare_statement(XSQLDA **insqlda, XSQLDA **outsqlda, char *query, isc_stmt_handle *stmtp, Transaction *atr);

  bool check_trans(Transaction **tr);
    
  void doref();
  
  void dounref();
  
  isc_db_handle get_DB_Handle();
  isc_tr_handle get_Def_Tr_Handle();

  void InstQuerySync(const Nan::FunctionCallbackInfo<v8::Value>& info, Transaction* transaction);
  void InstQuery(const Nan::FunctionCallbackInfo<v8::Value>& info, Transaction* transaction);
  void InstPrepareSync(const Nan::FunctionCallbackInfo<v8::Value>& info, Transaction* transaction);
 
 protected:
 
  static NAN_METHOD(New);
  static NAN_METHOD(ConnectSync);
 
  struct connect_request {
     Nan::Callback *callback;
     Connection *conn;
     Nan::Utf8String *Database;
     Nan::Utf8String *User;
     Nan::Utf8String *Password;
     Nan::Utf8String *Role;
	 Nan::Utf8String *lc_type;
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
   
  static NAN_METHOD(Commit);
  static NAN_METHOD(Rollback);
  static NAN_METHOD(Start);

  
  static NAN_METHOD(QuerySync);

  static NAN_METHOD(StartNewTransSync);
  static NAN_METHOD(StartNewTrans);

  struct query_request {
	 Nan::Callback *callback;
     Connection *conn;
	 Transaction* transaction;
     Nan::Utf8String *Query;
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

  static time_t  get_gmt_delta();

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
