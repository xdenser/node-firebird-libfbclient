/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#include <string.h>
#include <stdlib.h>
#include "./fb-bindings-fbresult.h"
#include "./fb-bindings-connection.h" 



void
  Connection::Initialize (v8::Handle<v8::Object> target)
  {
    HandleScope scope;
    
    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    
    t->Inherit(FBEventEmitter::constructor_template);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    // Methods 

    NODE_SET_PROTOTYPE_METHOD(t, "connectSync", ConnectSync);
    NODE_SET_PROTOTYPE_METHOD(t, "connect", Connect);
    NODE_SET_PROTOTYPE_METHOD(t, "querySync", QuerySync);
    NODE_SET_PROTOTYPE_METHOD(t, "query", Query);
    NODE_SET_PROTOTYPE_METHOD(t, "disconnect", Disconnect);
    NODE_SET_PROTOTYPE_METHOD(t, "commitSync", CommitSync);
    NODE_SET_PROTOTYPE_METHOD(t, "commit", Commit);
    NODE_SET_PROTOTYPE_METHOD(t, "rollbackSync", RollbackSync);
    NODE_SET_PROTOTYPE_METHOD(t, "rollback", Rollback);
    NODE_SET_PROTOTYPE_METHOD(t, "addFBevent", addEvent);
    NODE_SET_PROTOTYPE_METHOD(t, "deleteFBevent", deleteEvent);
    
    // Properties
    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
    instance_t->SetAccessor(String::NewSymbol("connected"), ConnectedGetter);
    instance_t->SetAccessor(String::NewSymbol("inTransaction"), InTransactionGetter);
    instance_t->SetAccessor(String::NewSymbol("inAsyncCall"),InAsyncGetter);
    
    target->Set(String::NewSymbol("Connection"), t->GetFunction());  
  }
  
bool Connection::Connect (const char* Database,const char* User,const char* Password,const char* Role)
  {
    if (db) return false;
    int i = 0, len;
    char dpb[256];
    char *lc_type = "UTF8";
    
    dpb[i++] = isc_dpb_version1;
    
    dpb[i++] = isc_dpb_user_name;
    len = strlen (User);
    dpb[i++] = (char) len;
    strncpy(&(dpb[i]), User, len);
    i += len;
    
    dpb[i++] = isc_dpb_password;
    len = strlen (Password);
    dpb[i++] = len;
    strncpy(&(dpb[i]), Password, len);
    i += len;

    dpb[i++] = isc_dpb_sql_role_name;
    len = strlen (Role);
    dpb[i++] = len;
    strncpy(&(dpb[i]), Role, len);
    i += len;
    
    dpb[i++] = isc_dpb_lc_ctype;
    len = strlen (lc_type);
    dpb[i++] = len;
    strncpy(&(dpb[i]), lc_type, len);
    i += len;
    
    connected = false;
    if(isc_attach_database(status, 0, Database, &(db), i, dpb)) return false;
    connected = true;
    return true;
  } 
  
  
bool Connection::Close(){
    
    if(trans)
     if(!commit_transaction()) {
       return false;
     }
    
    if(fb_events) free(fb_events);
    fb_events = NULL;
 
    connected = false; 
    if (isc_detach_database(status, &db)) {
      db = NULL;
      return false;
    } 
    db = NULL;
    return true;
    
  }
  
bool Connection::process_statement(XSQLDA **sqldap, char *query, isc_stmt_handle *stmtp)
  {
     XSQLDA          *sqlda;
     XSQLVAR         *var;
     static char     stmt_info[] = { isc_info_sql_stmt_type };
     char            info_buffer[20];
     short           l, num_cols, i, length, alignment, type, offset;
     int             statement_type;
     int 	     sqlda_size;	
     
     
     sqlda = *sqldap;
     
     if(!sqlda)
     {
          sqlda_size = XSQLDA_LENGTH (20);
          sqlda = (XSQLDA *) malloc(sqlda_size);
          memset(sqlda,0,sqlda_size);
          sqlda->sqln = 20;
          sqlda->version = 1;
          *sqldap = sqlda;
     }      

     /* Allocate a statement */
     if(!*stmtp) {
      if (isc_dsql_allocate_statement(status, &db, stmtp)) return false;
     } 
     
     // Start Default Transaction If None Active
     if(!trans) 
      {
      if (isc_start_transaction(status, &trans, 1, &db, 0, NULL)) return false;
      }
      
     // Prepare Statement
     if (isc_dsql_prepare(status, &trans, stmtp, 0, query, SQL_DIALECT_V6, sqlda)) return false;

     // Get sql info
     if (!isc_dsql_sql_info(status, stmtp, sizeof (stmt_info), stmt_info, 
         sizeof (info_buffer), info_buffer))
     {
         l = (short) isc_vax_integer((char *) info_buffer + 1, 2);
         statement_type = isc_vax_integer((char *) info_buffer + 3, l);
     }
     
     // It is statement w\o resultset
     if (!sqlda->sqld)
     {
        
        free(sqlda);
        *sqldap = NULL;
                   
        if (isc_dsql_execute(status, &trans, stmtp, SQL_DIALECT_V6, NULL))
        {
             return false;
        }

        /* Commit DDL statements if that is what sql_info says */

        if (trans && (statement_type == isc_info_sql_stmt_ddl))
        {
            //printf("it is ddl !");
            if (isc_commit_transaction(status, &trans))
            {
             trans = 0;
             return false;    
            }    
            trans = 0;
        }

        return true;
     }

     /*
      *    Process select statements.
      */

     num_cols = sqlda->sqld;

     /* Need more room. */
     if (sqlda->sqln < num_cols)
     {
        sqlda_size = XSQLDA_LENGTH (num_cols);
        *sqldap = sqlda = (XSQLDA *) realloc( sqlda, sqlda_size);
        memset(sqlda,0,sqlda_size);                                        
        sqlda->sqln = num_cols;
        sqlda->version = 1;

        if (isc_dsql_describe(status, stmtp, SQL_DIALECT_V6, sqlda))
        {
            return false;
        }

        num_cols = sqlda->sqld;
     }

     /*
      *     Set up SQLDA.
      */
    if(!FBResult::prepare_sqlda(sqlda)) return false;
      

    if (isc_dsql_execute(status, &trans, stmtp, SQL_DIALECT_V6, NULL))
    {
        return false;
    }
    return true;

  }
  

  
bool Connection::commit_transaction()
  {
    if (isc_commit_transaction(status, &trans))
    {
             trans = 0;
             return false;    
    }    
    trans = 0;
    return true;
  }
    
bool Connection::rollback_transaction()
  {
    if (isc_rollback_transaction(status, &trans))
    {
             trans = 0;
             return false;    
    }    
    trans = 0;
    return true;
  }
  
  
 
Handle<Value>
  Connection::New (const Arguments& args)
  {
    HandleScope scope;

    Connection *connection = new Connection();
    connection->Wrap(args.This());

    return args.This();
  }
  
Handle<Value>
  Connection::ConnectSync (const Arguments& args)
  {
    Connection *connection = ObjectWrap::Unwrap<Connection>(args.This());

    HandleScope scope;

    if (args.Length() < 4 || !args[0]->IsString() ||
         !args[1]->IsString() || !args[2]->IsString() ||
         !args[3]->IsString()) {
      return ThrowException(Exception::Error(
            String::New("Expecting 4 string arguments")));
    }

    String::Utf8Value Database(args[0]->ToString());
    String::Utf8Value User(args[1]->ToString());
    String::Utf8Value Password(args[2]->ToString());
    String::Utf8Value Role(args[3]->ToString());
    

    bool r = connection->Connect(*Database,*User,*Password,*Role);

    if (!r) {
      return ThrowException(Exception::Error(
            String::Concat(String::New("While connecting - "),ERR_MSG(connection, Connection))));
    }
    
    return Undefined();
  }

   
int Connection::EIO_After_Connect(eio_req *req)
  {
    ev_unref(EV_DEFAULT_UC);
    HandleScope scope;
    struct connect_request *conn_req = (struct connect_request *)(req->data);

    Local<Value> argv[1];
    
    if (!req->result) {
       argv[0] = Exception::Error(
            String::Concat(String::New("While connecting - "),ERR_MSG(conn_req->conn, Connection)));
    }
    else{
     argv[0] = Local<Value>::New(Null());
    }
   
    TryCatch try_catch;

    conn_req->callback->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    
    conn_req->callback.Dispose();
    conn_req->conn->stop_async();
    conn_req->conn->Unref();
    free(conn_req);
    return 0;
    
  }
  
int Connection::EIO_Connect(eio_req *req)
  {
    struct connect_request *conn_req = (struct connect_request *)(req->data);
    req->result = conn_req->conn->Connect(
                     **(conn_req->Database),
                     **(conn_req->User), 
                     **(conn_req->Password),
                     **(conn_req->Role));
    conn_req->res = req->result;
    delete conn_req->Database;
    delete conn_req->User;
    delete conn_req->Password;
    delete conn_req->Role;
    
    return 0;
   
  }
  
Handle<Value>
  Connection::Connect (const Arguments& args)
  {
    HandleScope scope;
    Connection *conn = ObjectWrap::Unwrap<Connection>(args.This());
    
    
    struct connect_request *conn_req =
         (struct connect_request *)calloc(1, sizeof(struct connect_request));

    if (!conn_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    if (args.Length() < 5 || !args[0]->IsString() ||
         !args[1]->IsString() || !args[2]->IsString() ||
         !args[3]->IsString() || !args[4]->IsFunction()) {
      return ThrowException(Exception::Error(
            String::New("Expecting 4 string arguments and 1 Function")));
    }
    
    conn_req->conn = conn;
    conn_req->callback = Persistent<Function>::New(Local<Function>::Cast(args[4]));
    conn_req->Database = new String::Utf8Value(args[0]->ToString());
    conn_req->User     = new String::Utf8Value(args[1]->ToString());
    conn_req->Password = new String::Utf8Value(args[2]->ToString());
    conn_req->Role     = new String::Utf8Value(args[3]->ToString());
    
    conn->start_async();
    
    eio_custom(EIO_Connect, EIO_PRI_DEFAULT, EIO_After_Connect, conn_req);
    
    ev_ref(EV_DEFAULT_UC);
    conn->Ref();
    
    return Undefined();
  }
  
Handle<Value>
  Connection::Disconnect(const Arguments& args)
  {
    Connection *connection = ObjectWrap::Unwrap<Connection>(args.This());

    HandleScope scope;
    //printf("disconnect\n");
    if(!connection->Close()){
      return ThrowException(Exception::Error(
            String::Concat(String::New("While closing - "),ERR_MSG(connection, Connection))));
    }     
   
    return Undefined();
  }
  
Handle<Value> 
  Connection::ConnectedGetter(Local<String> property,
                                      const AccessorInfo &info) 
  {
    HandleScope scope;
    Connection *connection = ObjectWrap::Unwrap<Connection>(info.Holder());

    return scope.Close(Boolean::New(connection->connected));
  }
  
Handle<Value>
  Connection::InTransactionGetter(Local<String> property,
                      const AccessorInfo &info) 
  {
    HandleScope scope;
    Connection *connection = ObjectWrap::Unwrap<Connection>(info.Holder());

    return scope.Close(Boolean::New(connection->trans));
  }
  
Handle<Value>
  Connection::CommitSync (const Arguments& args)
  {
    Connection *connection = ObjectWrap::Unwrap<Connection>(args.This());

    HandleScope scope;
    
    bool r = connection->commit_transaction();

    if (!r) {
      return ThrowException(Exception::Error(
            String::Concat(String::New("While commitSync - "),ERR_MSG(connection, Connection))));
    }
    
    return Undefined();
  } 
    
Handle<Value>
  Connection::RollbackSync (const Arguments& args)
  {
    Connection *connection = ObjectWrap::Unwrap<Connection>(args.This());

    HandleScope scope;
    
    bool r = connection->rollback_transaction();

    if (!r) {
      return ThrowException(Exception::Error(
            String::Concat(String::New("While rollbackSync - "),ERR_MSG(connection, Connection))));
    }
    
    return Undefined();
  } 
  
int Connection::EIO_After_TransactionRequest(eio_req *req)
  {
    ev_unref(EV_DEFAULT_UC);
    HandleScope scope;
    struct transaction_request *tr_req = (struct transaction_request *)(req->data);

    Local<Value> argv[1];
    
    if (!req->result) {
       argv[0] = Exception::Error(ERR_MSG(tr_req->conn, Connection));
    }
    else{
     argv[0] = Local<Value>::New(Null());
    }
   
    TryCatch try_catch;

    tr_req->callback->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    tr_req->callback.Dispose();
    tr_req->conn->stop_async();
    tr_req->conn->Unref();
    free(tr_req);

    return 0;
    
  }
  
int Connection::EIO_TransactionRequest(eio_req *req)
  {
    struct transaction_request *tr_req = (struct transaction_request *)(req->data);
    if(tr_req->commit) req->result = tr_req->conn->commit_transaction();
    else req->result = tr_req->conn->rollback_transaction();
    return 0;
  }

  
Handle<Value>
 Connection::Commit (const Arguments& args)
  {
    HandleScope scope;
    Connection *conn = ObjectWrap::Unwrap<Connection>(args.This());
    
    struct transaction_request *tr_req =
         (struct transaction_request *)calloc(1, sizeof(struct transaction_request));

    if (!tr_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    if (args.Length() < 1) {
      return ThrowException(Exception::Error(
            String::New("Expecting Callback Function argument")));
    }
    
    tr_req->conn = conn;
    tr_req->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));
    tr_req->commit = true;
    
    conn->start_async();
    eio_custom(EIO_TransactionRequest, EIO_PRI_DEFAULT, EIO_After_TransactionRequest, tr_req);
    
    ev_ref(EV_DEFAULT_UC);
    conn->Ref();
    
    return Undefined();
  }
  
Handle<Value>
  Connection::Rollback (const Arguments& args)
  {
    HandleScope scope;
    Connection *conn = ObjectWrap::Unwrap<Connection>(args.This());
    
    struct transaction_request *tr_req =
         (struct transaction_request *)calloc(1, sizeof(struct transaction_request));

    if (!tr_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    if (args.Length() < 1) {
      return ThrowException(Exception::Error(
            String::New("Expecting Callback Function argument")));
    }
    
    tr_req->conn = conn;
    tr_req->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));
    tr_req->commit = false;
    
    conn->start_async();
    eio_custom(EIO_TransactionRequest, EIO_PRI_DEFAULT, EIO_After_TransactionRequest, tr_req);
    
    ev_ref(EV_DEFAULT_UC);
    conn->Ref();
    
    return Undefined();
  } 
  
Handle<Value>
  Connection::QuerySync(const Arguments& args)
  {
    Connection *connection = ObjectWrap::Unwrap<Connection>(args.This());
    
    HandleScope scope;
    
    if (args.Length() < 1 || !args[0]->IsString()){
       return ThrowException(Exception::Error(
            String::New("Expecting a string query argument.")));
    }
    
    String::Utf8Value Query(args[0]->ToString());
    
    XSQLDA *sqlda = NULL;
    isc_stmt_handle stmt = NULL;
    bool r = connection->process_statement(&sqlda,*Query, &stmt);
    if(!r) {
      return ThrowException(Exception::Error(
            String::Concat(String::New("In querySync - "),ERR_MSG(connection, Connection))));
    }
    
    

    Local<Value> argv[2];

    argv[0] = External::New(sqlda);
    argv[1] = External::New(&stmt);
    Persistent<Object> js_result(FBResult::constructor_template->
                                     GetFunction()->NewInstance(2, argv));

    return scope.Close(js_result); 
        
  }
  
  
int Connection::EIO_After_Query(eio_req *req)
  {
    ev_unref(EV_DEFAULT_UC);
    HandleScope scope;
    struct query_request *q_req = (struct query_request *)(req->data);

    Local<Value> argv[2];
    
    if (!req->result) {
       argv[0] = Exception::Error(
            String::Concat(String::New("While query - "),ERR_MSG(q_req->conn, Connection)));
       argv[1] = Local<Value>::New(Null());        
    }
    else{
     argv[0] = External::New(q_req->sqlda);
     argv[1] = External::New(&q_req->stmt);
     Persistent<Object> js_result(FBResult::constructor_template->
                                     GetFunction()->NewInstance(2, argv));
     argv[1] = Local<Value>::New(scope.Close(js_result));    
     argv[0] = Local<Value>::New(Null());
    }

    TryCatch try_catch;

    q_req->callback->Call(Context::GetCurrent()->Global(), 2, argv);

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    q_req->callback.Dispose();
    q_req->conn->stop_async();
    q_req->conn->Unref();
    free(q_req);

    return 0;
    
  }
    
int Connection::EIO_Query(eio_req *req)
  {
    struct query_request *q_req = (struct query_request *)(req->data);
    
    req->result = q_req->conn->process_statement(&q_req->sqlda,**(q_req->Query), &q_req->stmt);
    delete q_req->Query;
    return 0;
  }
  
Handle<Value>
  Connection::Query(const Arguments& args)
  {
    HandleScope scope;
    Connection *conn = ObjectWrap::Unwrap<Connection>(args.This());
    
    struct query_request *q_req =
         (struct query_request *)calloc(1, sizeof(struct query_request));

    if (!q_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    if (args.Length() < 2 || !args[0]->IsString() ||
          !args[1]->IsFunction()) {
      return ThrowException(Exception::Error(
            String::New("Expecting 1 string argument and 1 Function")));
    }
    
    q_req->conn = conn;
    q_req->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));
    q_req->Query = new String::Utf8Value(args[0]->ToString());
    q_req->sqlda = NULL;
    q_req->stmt = 0;
    
    conn->start_async();
    eio_custom(EIO_Query, EIO_PRI_DEFAULT, EIO_After_Query, q_req);
    
    ev_ref(EV_DEFAULT_UC);
    conn->Ref();
    
    return Undefined();
    
  }
  
Handle<Value>
  Connection::addEvent(const Arguments& args)
  {
    
    
    HandleScope scope;
    Connection *conn = ObjectWrap::Unwrap<Connection>(args.This());
    
    if (args.Length() < 1 || !args[0]->IsString()) {
      return ThrowException(Exception::Error(
            String::New("Expecting 1 string argument")));
    }
    
    String::Utf8Value Event(args[0]->ToString());
    
    if(!event_block::FindBlock(conn->fb_events, *Event)){
        
        event_block* eb;
        
        return event_block::RegEvent(&(conn->fb_events), *Event, conn, &conn->db);
        
    }
    
    return Undefined();
  
  }
  
Handle<Value>
  Connection::deleteEvent(const Arguments& args)
  {
  
    HandleScope scope;
    Connection *conn = ObjectWrap::Unwrap<Connection>(args.This());
    
    if (args.Length() < 1 || !args[0]->IsString()) {
      return ThrowException(Exception::Error(
            String::New("Expecting 1 string argument")));
    }
    
    String::Utf8Value Event(args[0]->ToString());
    
    return event_block::RemoveEvent(&(conn->fb_events), *Event);

  }
  
    
   Connection::Connection () : FBEventEmitter () 
  {
    db = NULL;
    trans = NULL;
    fb_events = NULL;
    connected = false;
  }

  Connection::~Connection ()
  { 
    if(db!=NULL) Close();
    assert(db == NULL);
  }
 
