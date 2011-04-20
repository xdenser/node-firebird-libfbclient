/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#include <stdlib.h>
#include <string.h>
#include <v8.h>
#include <node.h>
#include <ibase.h>
#include "./fb-bindings-statement.h" 
#include "./fb-bindings-fbresult.h"

Persistent<FunctionTemplate> FBStatement::constructor_template;

void
 FBStatement::Initialize (v8::Handle<v8::Object> target)
 {
    HandleScope scope;
    
    error_symbol = NODE_PSYMBOL("error");
    result_symbol= NODE_PSYMBOL("result");
    
    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->Inherit(FBResult::constructor_template);
    
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(FBResult::constructor_template);
    constructor_template->SetClassName(String::NewSymbol("FBStatement"));

    Local<ObjectTemplate> instance_template =
        constructor_template->InstanceTemplate();
        
    NODE_SET_PROTOTYPE_METHOD(t, "execSync", ExecSync);
    NODE_SET_PROTOTYPE_METHOD(t, "exec", Exec);


    instance_template->SetInternalFieldCount(1);
    
//    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
//    instance_t->SetAccessor(String::NewSymbol("inAsyncCall"),InAsyncGetter);
    
    target->Set(String::NewSymbol("FBStatement"), t->GetFunction());
   
 }
 
Handle<Value>
  FBStatement::New (const Arguments& args)
  {
    HandleScope scope;
    
    REQ_EXT_ARG(0, js_in_sqldap);
    REQ_EXT_ARG(1, js_out_sqldap);
    REQ_EXT_ARG(2, js_stmtp);
    REQ_EXT_ARG(3, js_connection);
    
    XSQLDA *insqldap;
    insqldap = static_cast<XSQLDA *>(js_in_sqldap->Value());
    XSQLDA *outsqldap;
    outsqldap = static_cast<XSQLDA *>(js_out_sqldap->Value());
    isc_stmt_handle *astmtp;
    astmtp = static_cast<isc_stmt_handle *>(js_stmtp->Value()); 
    Connection  *conn;
    conn = static_cast<Connection *>(js_connection->Value()); 
    
    FBStatement *res = new FBStatement(insqldap,outsqldap, astmtp, conn);
    res->Wrap(args.This());

    return args.This();
  }

Handle<Value>
 FBStatement::ExecSync (const Arguments& args)
 { 
    HandleScope scope;
    
    FBStatement *fb_stmt = ObjectWrap::Unwrap<FBStatement>(args.This());
    
    FBResult::set_params(fb_stmt->in_sqlda, args);

    if(fb_stmt->retres) 
    {
      isc_dsql_free_statement(fb_stmt->status, &fb_stmt->stmt, DSQL_close);
      if (fb_stmt->status[1])
      {
        return ThrowException(Exception::Error(
           String::Concat(String::New("In FBStatement::execSync, free_statement - "),ERR_MSG(fb_stmt, FBStatement))));
      }
    }
    
        
    if (isc_dsql_execute(fb_stmt->status, &fb_stmt->connection->trans, &fb_stmt->stmt, SQL_DIALECT_V6, fb_stmt->in_sqlda))
    {
      return ThrowException(Exception::Error(
         String::Concat(String::New("In FBStatement::execSync - "),ERR_MSG(fb_stmt, FBStatement))));
    }
    
    if(!fb_stmt->sqldap->sqld) 
      return Undefined();
      
    fb_stmt->status[1]=0;
    isc_dsql_set_cursor_name(fb_stmt->status, &fb_stmt->stmt,fb_stmt->cursor,0);
    if (fb_stmt->status[1])
    {
      return ThrowException(Exception::Error(
      String::Concat(String::New("In FBStatement::execSync, set_cursor_name - "),ERR_MSG(fb_stmt, FBStatement))));
    }

    /*    
    Local<Value> argv[3];
    XSQLDA *sqlda = 0;
    if(!FBResult::clone_sqlda(fb_stmt->out_sqlda,&sqlda))
    {
      if(sqlda) free(sqlda);
      return ThrowException(Exception::Error(
         String::New("In FBStatement::execSync - cant clone SQLDA")));
    }
    argv[0] = External::New(sqlda);
    argv[1] = External::New(&fb_stmt->stmt);
    argv[2] = External::New(fb_stmt->conn);
    Persistent<Object> js_result(FBResult::constructor_template->
                                     GetFunction()->NewInstance(3, argv));
                                    
    return scope.Close(js_result); 
    */
    fb_stmt->retres = true;
    return Undefined();
    
 }
 
int FBStatement::EIO_After_Exec(eio_req *req)
 {
   ev_unref(EV_DEFAULT_UC);
   HandleScope scope;
   
   struct exec_request *e_req = (struct exec_request *)(req->data);
   FBStatement *fb_stmt = e_req->statement;
   Local<Value> argv[3];
   Handle<String> event;
   int argc = 0;
   if(!req->result)
   {
     argv[0] = Exception::Error(
         String::Concat(String::New("In FBStatement::EIO_After_Exec - "),ERR_MSG(fb_stmt, FBStatement)));
     argc = 0;
     event = error_symbol;     
     //((EventEmitter*) fb_stmt)->Emit(error_symbol,1,argv);           
   }
   else
   {
	if(!fb_stmt->sqldap->sqld) 
	{
    	    argc = 0; 
    	    event = result_symbol;     
        }
        else
        {
    	    fb_stmt->status[1]=0;
	    isc_dsql_set_cursor_name(fb_stmt->status, &fb_stmt->stmt,fb_stmt->cursor,0);
	    if (fb_stmt->status[1])
	    {		
    		argv[0] = Exception::Error(
    		String::Concat(String::New("In FBStatement::EIO_After_Exec, set_cursor_name - "),ERR_MSG(fb_stmt, FBStatement)));
    		argc = 1;
    		event = error_symbol;     
	    }
	    else
	    {
		/*XSQLDA *sqlda = 0;
		if(!FBResult::clone_sqlda(fb_stmt->out_sqlda,&sqlda))
		{
    		    if(sqlda) free(sqlda);
    		    argv[0] = Exception::Error(
        	       String::New("In FBStatement::EIO_After_Exec - cant clone SQLDA"));
		}
	        else
	        {
	    	        argv[0] = External::New(sqlda);
			argv[1] = External::New(&fb_stmt->stmt);
			argv[2] = External::New(fb_stmt->conn);
			Persistent<Object> js_result(FBResult::constructor_template->
                                    		      GetFunction()->NewInstance(3, argv));
			fb_stmt->retres = true; 
			argv[0] = Local<Value>::New(Null());
			argv[1] = Local<Value>::New(js_result);
			argc = 2;      
			event = result_symbol;                             
	        }*/
	        argv[0] = Local<Value>::New(Null());	
	        argc = 1;      
		event = result_symbol;                             
	    }
        }
       
   }
   ((EventEmitter*) fb_stmt)->Emit(event,argc,argv);           
    fb_stmt->stop_async();
    fb_stmt->Unref();
    free(e_req);
    return 0;
 }
    
int FBStatement::EIO_Exec(eio_req *req)
 {
    struct exec_request *e_req = (struct exec_request *)(req->data);
    FBStatement *fb_stmt = e_req->statement;
    
    if (isc_dsql_execute(fb_stmt->status, &fb_stmt->connection->trans, &fb_stmt->stmt, SQL_DIALECT_V6, fb_stmt->in_sqlda))
      req->result = false;
    else 
      req->result = true;
    
    return 0;
 }
 
Handle<Value>
 FBStatement::Exec (const Arguments& args)
 {
    HandleScope scope;
    FBStatement *fb_stmt = ObjectWrap::Unwrap<FBStatement>(args.This());
    
    struct exec_request *e_req =
         (struct exec_request *)calloc(1, sizeof(struct exec_request));

    if (!e_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    FBResult::set_params(fb_stmt->in_sqlda, args);

    if(fb_stmt->retres) 
    {
      isc_dsql_free_statement(fb_stmt->status, &fb_stmt->stmt, DSQL_close);
      if (fb_stmt->status[1])
      {
        return ThrowException(Exception::Error(
           String::Concat(String::New("In FBStatement::exec, free_statement - "),ERR_MSG(fb_stmt, FBStatement))));
      }
    }
    
    e_req->statement = fb_stmt;
    
    fb_stmt->start_async();
    eio_custom(EIO_Exec, EIO_PRI_DEFAULT, EIO_After_Exec, e_req);
    
    ev_ref(EV_DEFAULT_UC);
    fb_stmt->Ref();
    
    return Undefined();
 
 }
  
 FBStatement::FBStatement(XSQLDA *insqlda, XSQLDA *outsqlda, isc_stmt_handle *astmtp, Connection* aconn) : 
              FBResult (outsqlda,astmtp,aconn)
 {
//   conn = aconn;
   in_sqlda = insqlda;
 //  out_sqlda = outsqlda;
 //  stmt = *astmtp;
   retres = false;
   strncpy(cursor, "test_cursor", sizeof(cursor));
 } 
 
 FBStatement::~FBStatement()
 {
   if(in_sqlda) {
     FBResult::clean_sqlda(in_sqlda);
     free(in_sqlda);
   }
 /*  if(out_sqlda) {
     FBResult::clean_sqlda(out_sqlda);
     free(out_sqlda);
   } */
 }
 

