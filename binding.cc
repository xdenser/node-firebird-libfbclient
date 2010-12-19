#include <ibase.h>
#include <node.h>
#include <node_events.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "align.h"



using namespace v8;
using namespace node;

class Connection;

#define ERR_MSG(obj) \
String::New(Connection::ErrorMessage(obj->status,obj->err_message,sizeof(obj->err_message)))

#define REQ_EXT_ARG(I, VAR) \
if (args.Length() <= (I) || !args[I]->IsExternal()) \
return ThrowException(Exception::TypeError( \
String::New("Argument " #I " invalid"))); \
Local<External> VAR = Local<External>::Cast(args[I]);


class FBResult : public EventEmitter {

public:

// static Persistent<FunctionTemplate> constructor_template;
 
 static void
   Initialize (v8::Handle<v8::Object> target)
  {
    HandleScope scope;
    Persistent<FunctionTemplate> constructor_template;
    
    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->Inherit(EventEmitter::constructor_template);
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->SetClassName(String::NewSymbol("FBResult"));

    Local<ObjectTemplate> instance_template =
        constructor_template->InstanceTemplate();


    instance_template->SetInternalFieldCount(1);
    
    target->Set(String::NewSymbol("FBResult"), t->GetFunction());
  }
  
 bool process_result(XSQLDA **sqldap, isc_stmt_handle *stmtp, Local<Array> res)
  {
    int            fetch_stat;
    short 	   i, num_cols;	
    XSQLDA         *sqlda;
    
    uint32_t       j = 0;
    sqlda = *sqldap;
    if(!sqlda){ return true;}
    num_cols = (*sqldap)->sqld;
    
    HandleScope scope;
    
    Local<Object> js_result_row;
    Local<Value> js_field;
    
    while ((fetch_stat = isc_dsql_fetch(status, stmtp, SQL_DIALECT_V6, *sqldap)) == 0)
    {
        js_result_row = Array::New();
        for (i = 0; i < num_cols; i++)
        {
            js_field = FBResult::GetFieldValue((XSQLVAR *) &sqlda->sqlvar[i]);
            js_result_row->Set(Integer::NewFromUnsigned(i), js_field);
        }
        res->Set(Integer::NewFromUnsigned(j++), js_result_row);
    }
    return true;
  } 
  
protected:  
 static Handle<Value>
  New (const Arguments& args)
  {
  // XSQLDA **asqldap, isc_stmt_handle *astmtp
    HandleScope scope;
    
    REQ_EXT_ARG(0, js_sqldap);
    REQ_EXT_ARG(1, js_stmtp);
    
    XSQLDA **asqldap = static_cast<XSQLDA **>(js_sqldap->Value());
    isc_stmt_handle *astmtp = static_cast<isc_stmt_handle *>(js_stmtp->Value());
    FBResult *res = new FBResult(asqldap, astmtp);
    res->Wrap(args.This());

    return args.This();
  } 
  
  static Local<Value> 
  GetFieldValue(XSQLVAR *var)
  {
    short       dtype;  
    PARAMVARY   *vary2;
    short       len; 
    struct tm   times;
    ISC_QUAD    bid;
    time_t      rawtime;
    double      time_val;
    
    HandleScope scope;
    
    Local<Object> js_date;
    Local<Value> js_field = Local<Value>::New(Null());
    dtype = var->sqltype & ~1;
    if ((var->sqltype & 1) && (*var->sqlind < 0))
    {
     // NULL PROCESSING
    }
    else
    {
        switch (dtype)
        {
            case SQL_TEXT:
                js_field = String::New(var->sqldata,var->sqllen);
                break;

            case SQL_VARYING:
                vary2 = (PARAMVARY*) var->sqldata;
                vary2->vary_string[vary2->vary_length] = '\0';
                js_field = String::New((const char*)(vary2->vary_string));
                break;

            case SQL_SHORT:
            case SQL_LONG:
	    case SQL_INT64:
		{
		ISC_INT64	value;
		short		field_width;
		short		dscale;
		switch (dtype)
		    {
		    case SQL_SHORT:
			value = (ISC_INT64) *(short *) var->sqldata;
			field_width = 6;
			break;
		    case SQL_LONG:
			value = (ISC_INT64) *(int *) var->sqldata;
			field_width = 11;
			break;
		    case SQL_INT64:
			value = (ISC_INT64) *(ISC_INT64 *) var->sqldata;
			field_width = 21;
			break;
		    }
		ISC_INT64	tens;    
		short	i;
		dscale = var->sqlscale;
		
		if (dscale < 0)
		    {
		    tens = 1;
		    for (i = 0; i > dscale; i--) tens *= 10;
                    js_field = Number::New(value / tens); 
                    
		    }
		else if (dscale)
		      {
		       tens = 1;
		       for (i = 0; i < dscale; i++) tens *= 10;
		      js_field = Integer::New(value * tens); 
	              }		    
		else
		      js_field = Integer::New( value);
		}
                break;

            case SQL_FLOAT:
                js_field = Number::New(*(float *) (var->sqldata));  
                break;

            case SQL_DOUBLE:
                js_field = Number::New(*(double *) (var->sqldata));
                break;

            case SQL_TIMESTAMP: 
	            isc_decode_timestamp((ISC_TIMESTAMP *)var->sqldata, &times);
	            rawtime = mktime(&times);// + Connection::get_gmt_delta();
	            time_val = static_cast<double>(rawtime)*1000 + ((*((ISC_TIME *)var->sqldata)) % 10000)/10;
	            js_field = Date::New(time_val);
	            break;
	            
            case SQL_TYPE_DATE:    
	            	          
	            isc_decode_sql_date((ISC_DATE *)var->sqldata, &times);
	            rawtime = mktime(&times);// + Connection::get_gmt_delta();
	            js_date = Object::New();
	            js_date->Set(String::New("year"),
                             Integer::New(times.tm_year+1900));
                    js_date->Set(String::New("month"),
                             Integer::New(times.tm_mon+1));
                    js_date->Set(String::New("day"),
                             Integer::New(times.tm_mday));
	            time_val = static_cast<double>(rawtime)*1000;
	            js_date->Set(String::New("date"),
                             Date::New(time_val));
	            js_field = js_date;
	            break;
	            
           case SQL_TYPE_TIME:    
	            isc_decode_sql_time((ISC_TIME *)var->sqldata, &times);
	            time_val = (times.tm_hour*60*60 + 
	                        times.tm_min*60 +
	                        times.tm_sec)*1000 + 
	                        ((*((ISC_TIME *)var->sqldata)) % 10000)/10;
	            js_field = Date::New(time_val);            
	            break;            

            case SQL_BLOB:
            case SQL_ARRAY:
                /* Print the blob id on blobs or arrays */
                Local<Object> js_blob;
                bid = *(ISC_QUAD *) var->sqldata;
                js_blob = Object::New();
                js_blob->Set(String::New("q_hi"),
                             Integer::New(bid.gds_quad_high));
                js_blob->Set(String::New("q_lo"),
                             Integer::NewFromUnsigned(bid.gds_quad_low));             

                js_field = js_blob;
                
                break;

        }

    }
    
    return scope.Close(js_field);
  }
  
   FBResult (XSQLDA **asqldap, isc_stmt_handle *astmtp) : EventEmitter () 
  {
    sqldap = asqldap;
    stmtp = astmtp;
  }
  
  ~FBResult()
  {
  
  }
  
  ISC_STATUS_ARRAY status;
  char err_message[1024];    
  XSQLDA **sqldap;
  isc_stmt_handle *stmtp;
   
};

class Connection : public EventEmitter {
 public:
  static void
  Initialize (v8::Handle<v8::Object> target)
  {
    HandleScope scope;
    
    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    
    t->Inherit(EventEmitter::constructor_template);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    // Methods 

    NODE_SET_PROTOTYPE_METHOD(t, "connectSync", ConnectSync);
    NODE_SET_PROTOTYPE_METHOD(t, "connect", Connect);
    NODE_SET_PROTOTYPE_METHOD(t, "querySync", QuerySync);
    NODE_SET_PROTOTYPE_METHOD(t, "disconnect", Disconnect);
    // Properties
    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
    instance_t->SetAccessor(String::NewSymbol("connected"), ConnectedGetter);
    
    target->Set(String::NewSymbol("Connection"), t->GetFunction());  
  }
  
  bool Connect (const char* Database,const char* User,const char* Password,const char* Role)
  {
    if (db) return false;
    int i = 0, len;
    char dpb[256];
    
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
    connected = false;
    if(isc_attach_database(status, 0, Database, &(db), i, dpb)) return false;
    connected = true;
    return true;
  } 
  
  
  bool Close(){

    connected = false; 
    if (isc_detach_database(status, &db)) {
      db = NULL;
      return false;
    } 
    db = NULL;
    return true;
    
  }
  
  bool process_statement(XSQLDA **sqldap, char *query, isc_stmt_handle *stmtp)
  {
     int            buffer[1024];
     isc_stmt_handle stmt = *stmtp;
     XSQLDA          *sqlda;
     XSQLVAR         *var;
     static char     stmt_info[] = { isc_info_sql_stmt_type };
     char            info_buffer[20];
     short           l, num_cols, i, length, alignment, type, offset;
     int             statement_type;
     
     
     sqlda = *sqldap;
     
     if(!sqlda)
     {
          sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH (20));
          sqlda->sqln = 20;
          sqlda->version = 1;
          *sqldap = sqlda;
     }      

     /* Allocate a statement */
     if(!stmt) {
      if (isc_dsql_allocate_statement(status, &db, &stmt)) return false;
      *stmtp = stmt;  
     } 
     
     // Start Default Transaction If None Active
     if(!trans) 
      if (isc_start_transaction(status, &trans, 1, &db, 0, NULL)) return false;
      
     // Prepare Statement
     if (isc_dsql_prepare(status, &trans, &stmt, 0, query, SQL_DIALECT_V6, sqlda)) return false;

     // Get sql info
     if (!isc_dsql_sql_info(status, &stmt, sizeof (stmt_info), stmt_info, 
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
                   
        if (isc_dsql_execute(status, &trans, &stmt, SQL_DIALECT_V6, NULL))
        {
             return false;
        }

        /* Commit DDL statements if that is what sql_info says */

        if (trans && (statement_type == isc_info_sql_stmt_ddl))
        {
            
            if (isc_commit_transaction(status, &trans))
            {
             return false;    
            }    
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
        *sqldap = sqlda = (XSQLDA *) realloc(sqlda,
                                                XSQLDA_LENGTH (num_cols));
        sqlda->sqln = num_cols;
        sqlda->version = 1;

        if (isc_dsql_describe(status, &stmt, SQL_DIALECT_V6, sqlda))
        {
            return false;
        }

        num_cols = sqlda->sqld;
     }

     /*
      *     Set up SQLDA.
      */
     for (var = sqlda->sqlvar, offset = 0, i = 0; i < num_cols; var++, i++)
     {
        length = alignment = var->sqllen;
        type = var->sqltype & ~1;

        if (type == SQL_TEXT)
            alignment = 1;
        else if (type == SQL_VARYING)
        {   
            length += sizeof (short) + 1;
            alignment = sizeof (short);
        }
        /* Comment from firebird sample: 
        **  RISC machines are finicky about word alignment
        **  So the output buffer values must be placed on
        **  word boundaries where appropriate
        */
        offset = FB_ALIGN(offset, alignment);
        var->sqldata = (char *) buffer + offset;
        offset += length;
        offset = FB_ALIGN(offset, sizeof (short));
        var->sqlind = (short*) ((char *) buffer + offset);
        offset += sizeof  (short);
     }

    if (isc_dsql_execute(status, &trans, &stmt, SQL_DIALECT_V6, NULL))
    {
        return false;
    }
    
    return true;

  }
  
  bool start_transaction()
  {
     
  }
  
   
  
  bool commit_transaction()
  {
    if (isc_commit_transaction(status, &trans))
    {
             return false;    
    }    
    return true;
  }
  
  bool process_result(XSQLDA **sqldap, isc_stmt_handle *stmtp, Local<Array> res)
  {
    int            fetch_stat;
    short 	   i, num_cols;	
    XSQLDA         *sqlda;
    
    uint32_t       j = 0;
    sqlda = *sqldap;
    if(!sqlda){ return true;}
    num_cols = (*sqldap)->sqld;
    
    HandleScope scope;
    
    Local<Object> js_result_row;
    Local<Value> js_field;
    
    while ((fetch_stat = isc_dsql_fetch(status, stmtp, SQL_DIALECT_V6, *sqldap)) == 0)
    {
        js_result_row = Array::New();
        for (i = 0; i < num_cols; i++)
        {
            js_field = Connection::GetFieldValue((XSQLVAR *) &sqlda->sqlvar[i]);
            js_result_row->Set(Integer::NewFromUnsigned(i), js_field);
        }
        res->Set(Integer::NewFromUnsigned(j++), js_result_row);
    }
    return true;
  }
  
  static char * ErrorMessage(const ISC_STATUS *pvector, char *err_msg, int max_len){
    err_msg[0] = 0;
    char s[512], *p, *t;
    t = err_msg;
    while(fb_interpret(s,sizeof(s),&pvector)){
      for(p=s;(*p)&&((t-err_msg)<max_len);){ *t++ = *p++; }
      if((t-err_msg+1)<max_len) *t++='\n';
    };
    if((t-err_msg)<max_len) *t = 0;
    return err_msg;
  }
  
 protected:
 
 static Handle<Value>
  New (const Arguments& args)
  {
    HandleScope scope;

    Connection *connection = new Connection();
    connection->Wrap(args.This());

    return args.This();
  }
  
  static Handle<Value>
  ConnectSync (const Arguments& args)
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
            String::Concat(String::New("While connecting - "),ERR_MSG(connection))));
    }
    
    return Undefined();
  }

  struct connect_request {
     Persistent<Function> callback;
     Connection *conn;
     String::Utf8Value *Database;
     String::Utf8Value *User;
     String::Utf8Value *Password;
     String::Utf8Value *Role;
     bool res;
  };
  
  static int EIO_After_Connect(eio_req *req)
  {
    ev_unref(EV_DEFAULT_UC);
    HandleScope scope;
    struct connect_request *conn_req = (struct connect_request *)(req->data);

    Local<Value> argv[1];
    
    if (!req->result) {
       argv[0] = Exception::Error(
            String::Concat(String::New("While connecting - "),ERR_MSG(conn_req->conn)));
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
    conn_req->conn->Unref();
    free(conn_req);

    return 0;
    
  }
  
  static int EIO_Connect(eio_req *req)
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
  
  static Handle<Value>
  Connect (const Arguments& args)
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
    
    eio_custom(EIO_Connect, EIO_PRI_DEFAULT, EIO_After_Connect, conn_req);
    
    ev_ref(EV_DEFAULT_UC);
    conn->Ref();
    
    return Undefined();
  }
  
  static Handle<Value>
  Disconnect(const Arguments& args)
  {
    Connection *connection = ObjectWrap::Unwrap<Connection>(args.This());

    HandleScope scope;
    
    if(!connection->Close()){
      return ThrowException(Exception::Error(
            String::Concat(String::New("While closing - "),ERR_MSG(connection))));
    }     
   
    return Undefined();
  }
  
  static Handle<Value> ConnectedGetter(Local<String> property,
                                      const AccessorInfo &info) {
    HandleScope scope;
    Connection *connection = ObjectWrap::Unwrap<Connection>(info.Holder());

    return scope.Close(Boolean::New(connection->connected));
  }

  
  static Handle<Value>
  QuerySync(const Arguments& args)
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
    //printf("qsync %s\n",*Query);
    if(!r) {
      return ThrowException(Exception::Error(
            String::Concat(String::New("In querySync - "),ERR_MSG(connection))));
    }
    
    

    Local<Value> argv[2];

    argv[0] = External::New(&sqlda);
    argv[1] = External::New(&stmt);
 //    XSQLDA **asqldap, isc_stmt_handle *astmtp
    Persistent<Object> js_result(FBResult::constructor_template->
                                     GetFunction()->NewInstance(2, argv));
    
    if(connection->trans)
     if(!connection->commit_transaction()) {
      return ThrowException(Exception::Error(
            String::Concat(String::New("In commit_transaction - "),ERR_MSG(connection))));
     } 
     

    //printf("exit qsync \n");    
    return scope.Close(js_result); 
        
  }

  static time_t 
  get_gmt_delta()
  {
              int h1 = 0, h2 = 0, m1 = 0, m2 = 0, d1 = 0, d2 = 0;
              time_t rawtime;
              struct tm timeinfo;
              time(&rawtime);
              if (!localtime_r(&rawtime, &timeinfo)) {
                  return 0;
              }
              h1 = timeinfo.tm_hour - (timeinfo.tm_isdst > 0 ? 1 : 0);
              m1 = timeinfo.tm_min;
              d1 = timeinfo.tm_mday;
              if (!gmtime_r(&rawtime, &timeinfo)) {
                  return 0;
              }
              h2 = timeinfo.tm_hour;
              m2 = timeinfo.tm_min;
              d2 = timeinfo.tm_mday;
              return (((h1 - h2)%24)*60 + (m1-m2))*60;
               
  }
  
  static time_t
  get_time(struct tm *tm_p)
  {
     
  }
  
  static Local<Value> 
  GetFieldValue(XSQLVAR *var)
  {
    short       dtype;  
    PARAMVARY   *vary2;
    short       len; 
    struct tm   times;
    ISC_QUAD    bid;
    time_t      rawtime;
    double      time_val;
    
    HandleScope scope;
    
    Local<Object> js_date;
    Local<Value> js_field = Local<Value>::New(Null());
    dtype = var->sqltype & ~1;
    if ((var->sqltype & 1) && (*var->sqlind < 0))
    {
     // NULL PROCESSING
    }
    else
    {
        switch (dtype)
        {
            case SQL_TEXT:
                js_field = String::New(var->sqldata,var->sqllen);
                break;

            case SQL_VARYING:
                vary2 = (PARAMVARY*) var->sqldata;
                vary2->vary_string[vary2->vary_length] = '\0';
                js_field = String::New((const char*)(vary2->vary_string));
                break;

            case SQL_SHORT:
            case SQL_LONG:
	    case SQL_INT64:
		{
		ISC_INT64	value;
		short		field_width;
		short		dscale;
		switch (dtype)
		    {
		    case SQL_SHORT:
			value = (ISC_INT64) *(short *) var->sqldata;
			field_width = 6;
			break;
		    case SQL_LONG:
			value = (ISC_INT64) *(int *) var->sqldata;
			field_width = 11;
			break;
		    case SQL_INT64:
			value = (ISC_INT64) *(ISC_INT64 *) var->sqldata;
			field_width = 21;
			break;
		    }
		ISC_INT64	tens;    
		short	i;
		dscale = var->sqlscale;
		
		if (dscale < 0)
		    {
		    tens = 1;
		    for (i = 0; i > dscale; i--) tens *= 10;
                    js_field = Number::New(value / tens); 
                    
		    }
		else if (dscale)
		      {
		       tens = 1;
		       for (i = 0; i < dscale; i++) tens *= 10;
		      js_field = Integer::New(value * tens); 
	              }		    
		else
		      js_field = Integer::New( value);
		}
                break;

            case SQL_FLOAT:
                js_field = Number::New(*(float *) (var->sqldata));  
                break;

            case SQL_DOUBLE:
                js_field = Number::New(*(double *) (var->sqldata));
                break;

            case SQL_TIMESTAMP: 
	            isc_decode_timestamp((ISC_TIMESTAMP *)var->sqldata, &times);
	            rawtime = mktime(&times);// + Connection::get_gmt_delta();
	            time_val = static_cast<double>(rawtime)*1000 + ((*((ISC_TIME *)var->sqldata)) % 10000)/10;
	            js_field = Date::New(time_val);
	            break;
	            
            case SQL_TYPE_DATE:    
	            	          
	            isc_decode_sql_date((ISC_DATE *)var->sqldata, &times);
	            rawtime = mktime(&times);// + Connection::get_gmt_delta();
	            js_date = Object::New();
	            js_date->Set(String::New("year"),
                             Integer::New(times.tm_year+1900));
                    js_date->Set(String::New("month"),
                             Integer::New(times.tm_mon+1));
                    js_date->Set(String::New("day"),
                             Integer::New(times.tm_mday));
	            time_val = static_cast<double>(rawtime)*1000;
	            js_date->Set(String::New("date"),
                             Date::New(time_val));
	            js_field = js_date;
	            break;
	            
           case SQL_TYPE_TIME:    
	            isc_decode_sql_time((ISC_TIME *)var->sqldata, &times);
	            time_val = (times.tm_hour*60*60 + 
	                        times.tm_min*60 +
	                        times.tm_sec)*1000 + 
	                        ((*((ISC_TIME *)var->sqldata)) % 10000)/10;
	            js_field = Date::New(time_val);            
	            break;            

            case SQL_BLOB:
            case SQL_ARRAY:
                /* Print the blob id on blobs or arrays */
                Local<Object> js_blob;
                bid = *(ISC_QUAD *) var->sqldata;
                js_blob = Object::New();
                js_blob->Set(String::New("q_hi"),
                             Integer::New(bid.gds_quad_high));
                js_blob->Set(String::New("q_lo"),
                             Integer::NewFromUnsigned(bid.gds_quad_low));             

                js_field = js_blob;
                
                break;

        }

    }
    
    return scope.Close(js_field);
  }
    
   Connection () : EventEmitter () 
  {
    db = NULL;
    trans = NULL;
    in_async = false;
    connected = false;

//    connecting_ = resetting_ = false;

    ev_init(&read_watcher_, io_event);
    read_watcher_.data = this;

    ev_init(&write_watcher_, io_event);
    write_watcher_.data = this;
  }

  ~Connection ()
  {
    //printf("conn destroy\n");
    if(db!=NULL) Close();
    assert(db == NULL);
  }
 
 
 private:
    void Event (int revents)
  {

  }

 
  static void
  io_event (EV_P_ ev_io *w, int revents)
  {
    Connection *connection = static_cast<Connection*>(w->data);
    connection->Event(revents);
  }

 
  ev_io read_watcher_;
  ev_io write_watcher_;

  ISC_STATUS_ARRAY status;
  isc_db_handle db;
  bool connected; 
  isc_tr_handle trans;
  bool in_async;
  char err_message[1024];

};

extern "C" void
init (Handle<Object> target) 
{
  HandleScope scope;
  FBResult::Initialize(target);
  Connection::Initialize(target);
}
